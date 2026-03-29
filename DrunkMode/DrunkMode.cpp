#include "DrunkMode.h"

IMod* BMLEntry(IBML* bml) {
    return new DrunkMode(bml);
}

// Intensity: 整体效果强度
// Frequency: 晃动频率
// DisplacementScale : 相机位移幅度

void DrunkMode::OnLoad() {
    mod_enabled_config = GetConfig()->GetProperty("Main", "Enabled");
    mod_enabled_config->SetDefaultBoolean(true);
    mod_enabled_config->SetComment("Turn on to use this mod.");

    camera_enabled_config = GetConfig()->GetProperty("Camera", "Camera_Enabled");
    camera_enabled_config->SetDefaultBoolean(true);
    camera_enabled_config->SetComment("Turn on to use camera effects");

    camera_intensity_config = GetConfig()->GetProperty("Camera", "Camera_Intensity");
    camera_intensity_config->SetComment("Overall effect intensity. Default: 1");
    camera_intensity_config->SetDefaultFloat(1.0f);

    camera_frequency_config = GetConfig()->GetProperty("Camera", "Camera_Frequency");
    camera_frequency_config->SetComment("Shaking frequency of camera. Default: 1");
    camera_frequency_config->SetDefaultFloat(1.0f);

    camera_displacement_scale_config = GetConfig()->GetProperty("Camera", "Camera_Displacement");
    camera_displacement_scale_config->SetComment("Camera displacement magnitude. Default: 1");
    camera_displacement_scale_config->SetDefaultFloat(1.0f);

    force_enabled_config = GetConfig()->GetProperty("Force", "Force_Enabled");
    force_enabled_config->SetDefaultBoolean(true);
    force_enabled_config->SetComment("Turn on to use the runaway effect");

    force_intensity_config = GetConfig()->GetProperty("Force", "Force_Intensity");
    force_intensity_config->SetComment("Overall uncontrollable random force magnitude. Default: 1");
    force_intensity_config->SetDefaultFloat(1.0f);

    force_frequency_config = GetConfig()->GetProperty("Force", "Force_Frequency");
    force_frequency_config->SetComment("The speed of uncontrolled random force variations. Default: 1");
    force_frequency_config->SetDefaultFloat(1.0f);


    custom_camera = static_cast<CKCamera*>(m_bml->GetCKContext()->CreateObject(CKCID_CAMERA, "DrunkModeCamera"));

    load_config();
}

void DrunkMode::ApplyDrunkEffect() {
    float deltaTime = m_BML->GetTimeManager()->GetLastDeltaTime() / 1000.0f;
    camera_TimeAccumulator += deltaTime * camera_frequency;

    // 时间规范化
    if (camera_TimeAccumulator > 1000.0f)
        camera_TimeAccumulator -= 1000.0f;

    float intensity = camera_intensity;
    float dispScale = camera_displacement_scale / 10.0f;

    // 多频振荡值
    float sin1 = sin(camera_TimeAccumulator);
    float sin2 = sin(camera_TimeAccumulator * 2.1f);
    float cos1 = cos(camera_TimeAccumulator * 1.7f);
    float cos2 = cos(camera_TimeAccumulator * 0.8f);

    // 旋转分量
    float roll = sin1 * 0.08f * intensity;
    float pitch = (sin2 * 0.04f + cos2 * 0.02f) * intensity;
    float yaw = (sin1 * 0.03f + cos1 * 0.05f) * intensity;

    // 位移分量
    VxVector displacement(
        (sin2 * 0.2f + cos1 * 0.15f) * dispScale * intensity,
        (sin1 * 0.1f + cos2 * 0.25f) * dispScale * intensity,
        0  // 避免前后位移防止眩晕
    );

    // 应用位移
    VxVector newPos2;
    m_BML->GetTargetCameraByName("InGameCam")->GetPosition(&newPos2);
    VxVector newPos = newPos2 + displacement;

    // 创建旋转轴
    VxVector axisPitch(1, 0, 0);  // X轴 - 俯仰
    VxVector axisYaw(0, 1, 0);    // Y轴 - 偏航
    VxVector axisRoll(0, 0, 1);   // Z轴 - 滚动

    // 创建基础旋转
    VxQuaternion totalRotation(0, 0, 0, 1);

    // 添加旋转分量
    if (abs(yaw) > 0.001f) {
        VxQuaternion yawRot = CreateQuaternionFromAxisAngle(axisYaw, yaw);
        totalRotation = MultiplyQuaternions(yawRot, totalRotation);
    }
    if (abs(pitch) > 0.001f) {
        VxQuaternion pitchRot = CreateQuaternionFromAxisAngle(axisPitch, pitch);
        totalRotation = MultiplyQuaternions(pitchRot, totalRotation);
    }
    if (abs(roll) > 0.001f) {
        VxQuaternion rollRot = CreateQuaternionFromAxisAngle(axisRoll, roll);
        totalRotation = MultiplyQuaternions(rollRot, totalRotation);
    }

    // 组合最终旋转
    VxQuaternion newRot2;
    m_BML->GetTargetCameraByName("InGameCam")->GetQuaternion(&newRot2);
    VxQuaternion newRot = MultiplyQuaternions(totalRotation, newRot2);
    newRot.Normalize();

    m_BML->GetRenderContext()->AttachViewpointToCamera(custom_camera);
    custom_camera->SetFov(m_BML->GetTargetCameraByName("InGameCam")->GetFov());
    custom_camera->SetPosition(&newPos);
    custom_camera->SetQuaternion(&newRot);

}

void DrunkMode::ApplyNonLinearRandomXZForce() {
    float deltaTime = m_BML->GetTimeManager()->GetLastDeltaTime() / 1000.0f;
    force_TimeAccumulator += deltaTime * force_frequency;

    if (force_TimeAccumulator > 1000.0f)
        force_TimeAccumulator -= 1000.0f;


    CKDataArray* current_level_array_ = m_BML->GetArrayByName("CurrentLevel");
    CK3dEntity* currentBall = static_cast<CK3dObject*>((current_level_array_)->GetElementObject(0, 1));

    std::unordered_map<std::string, float> ball_name_to_quality{ {"Ball_Paper", 0.3f }, {"Ball_Wood", 1.9f}, {"Ball_Stone", 5.4f} }; // 0.2 | 1.9 | 10
    float ball_quality = ball_name_to_quality[currentBall->GetName()];



    constexpr float GOLDEN_RATIO = 1.618f;
    float x_dir =
        std::sin(force_TimeAccumulator * GOLDEN_RATIO * 0.8f) +
        std::cos(force_TimeAccumulator * GOLDEN_RATIO * 1.2f) +
        std::sin(force_TimeAccumulator * GOLDEN_RATIO * 0.3f);
    float z_dir =
        std::cos(force_TimeAccumulator * GOLDEN_RATIO * 1.1f) +
        std::sin(force_TimeAccumulator * GOLDEN_RATIO * 0.7f) +
        std::cos(force_TimeAccumulator * GOLDEN_RATIO * 0.5f);


    VxVector direction(x_dir, 0.0f, z_dir);
    if (direction.SquareMagnitude() > 0.001f) {
        direction.Normalize();
    }
    else {
        return;
    }

    float force_ = (
        std::sin(force_TimeAccumulator * GOLDEN_RATIO * 1.1f) +
        std::cos(force_TimeAccumulator * GOLDEN_RATIO * 1.3f) +
        std::cos(force_TimeAccumulator * GOLDEN_RATIO * 0.3f));

    float impulse = force_intensity * ball_quality * force_ * 0.022f;

    // 参数说明: 目标实体, 施力点(默认0,0,0代表球心), 施力点参考系, 方向, 方向参考系, 冲量大小
    ExecuteBB::PhysicsImpulse(currentBall, VxVector(0, 0, 0), currentBall, direction, nullptr, impulse);
    ExecuteBB::PhysicsWakeUp(currentBall);
}

void DrunkMode::OnProcess() {
    if (!mod_enabled || !m_BML->IsPlaying()) { return; }

    // 摄像机摇晃
    if (camera_enabled){
        ApplyDrunkEffect();
    }
    // 受力失控：给予随机非线性变化外力
    if (force_enabled) {
        ApplyNonLinearRandomXZForce();
    }

    // OnDrawInfo();

}

/* 
void DrunkMode::OnDrawInfo() {
    constexpr ImGuiWindowFlags WinFlags = ImGuiWindowFlags_AlwaysAutoResize |
        ImGuiWindowFlags_NoDecoration |
        ImGuiWindowFlags_NoNav |
        ImGuiWindowFlags_NoFocusOnAppearing |
        ImGuiWindowFlags_NoBringToFrontOnFocus |
        ImGuiWindowFlags_NoSavedSettings;

    // 设置窗口在右上角显示
    // 锚点(1.0, 0.0)表示右上角，间距(10,10)提供一些边距
    ImGui::SetNextWindowPos(ImVec2(ImGui::GetIO().DisplaySize.x - 10, 10), ImGuiCond_Always, ImVec2(1.0f, 0.0f));

    if (ImGui::Begin("Camera Info", nullptr, WinFlags)) {
        VxVector VxVector_info;
        VxQuaternion VxQuaternion_info;

        m_BML->Get3dEntityByName("Cam_Target")->GetPosition(&VxVector_info);
        ImGui::Text("Cam_Target Position\n%s", VxVectorToString(VxVector_info).c_str());

        m_BML->GetTargetCameraByName("InGameCam")->GetQuaternion(&VxQuaternion_info);
        ImGui::Text("InGameCam Quaternion\n%s", VxQuaternionToString(VxQuaternion_info).c_str());

        m_BML->GetTargetCameraByName("InGameCam")->GetPosition(&VxVector_info);
        ImGui::Text("InGameCam Position\n%s", VxVectorToString(VxVector_info).c_str());

        float m_ = m_BML->GetTargetCameraByName("InGameCam")->GetFov();
        ImGui::Text("InGameCam Fov\n%.3f", m_);

        m_BML->Get3dEntityByName("Cam_OrientRef")->GetQuaternion(&VxQuaternion_info);
        ImGui::Text("Cam_OrientRef Quaternion\n%s", VxQuaternionToString(VxQuaternion_info).c_str());

        m_BML->Get3dEntityByName("Cam_OrientRef")->GetPosition(&VxVector_info);
        ImGui::Text("Cam_OrientRef Position\n%s", VxVectorToString(VxVector_info).c_str());

        m_BML->Get3dEntityByName("Cam_Orient")->GetQuaternion(&VxQuaternion_info);
        ImGui::Text("Cam_Orient Quaternion\n%s", VxQuaternionToString(VxQuaternion_info).c_str());

        m_BML->Get3dEntityByName("Cam_Orient")->GetPosition(&VxVector_info);
        ImGui::Text("Cam_Orient Position\n%s", VxVectorToString(VxVector_info).c_str());

        custom_camera->GetQuaternion(&VxQuaternion_info);
        ImGui::Text("custom_camera Quaternion\n%s", VxQuaternionToString(VxQuaternion_info).c_str());

        custom_camera->GetPosition(&VxVector_info);
        ImGui::Text("custom_camera Position\n%s", VxVectorToString(VxVector_info).c_str());
    }
    ImGui::End();
}
*/