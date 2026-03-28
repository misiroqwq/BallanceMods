#include "ControlDirCamera.h"

IMod* BMLEntry(IBML* bml) {
    return new ControlDirCamera(bml);
}
void ControlDirCamera::OnLoad() {
    mod_enabled_config = GetConfig()->GetProperty("Main", "Enabled");
    mod_enabled_config->SetDefaultBoolean(true);
    mod_enabled_config->SetComment("Enabled to use this mod.");

    showdata_enabled_config = GetConfig()->GetProperty("Main", "Show_Data");
    showdata_enabled_config->SetDefaultBoolean(false);
    showdata_enabled_config->SetComment("Enabled to show camera data on upper right corner of the screen when playing.");

    camera_fov_config = GetConfig()->GetProperty("Main", "Camera_Fov");
    camera_fov_config->SetDefaultFloat(1.0f);
    camera_fov_config->SetComment("Changes the field of view of the camera. Default: 1.0 | value range: 0-N/A");
    
    camera_angle_right_config = GetConfig()->GetProperty("Main", "Camera_Angle_Right");
    camera_angle_right_config->SetDefaultFloat(0.0f);
    camera_angle_right_config->SetComment("Horizontal offset angle of the camera. Default: 0.0 | value range: 0-360");

    camera_angle_up_config = GetConfig()->GetProperty("Main", "Camera_Angle_UP");
    camera_angle_up_config->SetDefaultFloat(0.766f);
    camera_angle_up_config->SetComment("The vertical angle of the camera's line of sight. Default: 0.766 | value range: 0-1");

    camera_height_config = GetConfig()->GetProperty("Main", "Camera_Height");
    camera_height_config->SetDefaultFloat(35.0f);
    camera_height_config->SetComment("the height of the camera. Default: 35.0");

    custom_camera = static_cast<CKCamera*>(m_bml->GetCKContext()->CreateObject(CKCID_CAMERA, "ControlDirCamera"));

    load_config();
}

VxVector ControlDirCamera::CalculateCameraPosition(const VxVector& ballPosition, const VxQuaternion& viewQuaternion, float angle) {
    // 1. 定义局部基准偏移量 (基于前期推导)
    VxVector baseOffset = DEFAULT_POSITION_CUSTOM_CAMERA_OFFSET;

    // 2. 求视口四元数的共轭 (Conjugate)，用于从摄像机空间转到世界空间
    float invX = -viewQuaternion.x;
    float invY = -viewQuaternion.y;
    float invZ = -viewQuaternion.z;
    float invW = viewQuaternion.w;

    // 3. 计算四元数旋转后的世界空间偏移量
    VxVector rotatedOffset;
    rotatedOffset.x = baseOffset.x * (1.0f - 2.0f * invY * invY - 2.0f * invZ * invZ) +
        baseOffset.y * (2.0f * invX * invY - 2.0f * invW * invZ) +
        baseOffset.z * (2.0f * invX * invZ + 2.0f * invW * invY);

    rotatedOffset.y = baseOffset.x * (2.0f * invX * invY + 2.0f * invW * invZ) +
        baseOffset.y * (1.0f - 2.0f * invX * invX - 2.0f * invZ * invZ) +
        baseOffset.z * (2.0f * invY * invZ - 2.0f * invW * invX);

    rotatedOffset.z = baseOffset.x * (2.0f * invX * invZ - 2.0f * invW * invY) +
        baseOffset.y * (2.0f * invY * invZ + 2.0f * invW * invX) +
        baseOffset.z * (1.0f - 2.0f * invX * invX - 2.0f * invY * invY);

    // 4. 处理 XZ 平面的附加旋转 (如果 angle 不为 0 或 360)
    // 浮点数比较建议使用一个很小的 epsilon 范围，这里简化处理直接判断
    if (angle != 0.0f && angle != 360.0f) {
        // 角度转弧度
        float rad = angle * PI / 180.0f;
        float cosA = std::cos(rad);
        float sinA = std::sin(rad);

        // 应用 2D 旋转矩阵到 X 和 Z 分量
        // 经典的二维旋转公式：x' = x*cos(θ) - z*sin(θ), z' = x*sin(θ) + z*cos(θ)
        float finalOffsetX = rotatedOffset.x * cosA - rotatedOffset.z * sinA;
        float finalOffsetZ = rotatedOffset.x * sinA + rotatedOffset.z * cosA;

        rotatedOffset.x = finalOffsetX;
        rotatedOffset.z = finalOffsetZ;
    }

    // 5. 球的坐标 + 最终旋转后的偏移量 = 摄像机坐标
    VxVector cameraPosition;
    cameraPosition.x = ballPosition.x + rotatedOffset.x;
    cameraPosition.y = ballPosition.y + rotatedOffset.y;
    cameraPosition.z = ballPosition.z + rotatedOffset.z;

    return cameraPosition;
}

VxQuaternion ControlDirCamera::CalculateCameraQuaternion(VxQuaternion& quaternion_Cam_OrientRef, float camera_angle_y) {
    float radian = camera_angle_y * PI / 180.0f;
    VxVector yAxis(0.0f, 1.0f, 0.0f);
    VxQuaternion rotY;
    rotY.FromRotation(yAxis, radian);
    VxQuaternion result;
    result = DEFAULT_QUATERNION_CUSTOM_CAMERA_OFFSET * rotY * quaternion_Cam_OrientRef;
    result.Normalize();
    return result;
}

void ControlDirCamera::OnProcess() {
    if (!mod_enabled || !m_BML->IsPlaying()) {return;}
    // m_CamPos->SetPosition(&m_OriginalPos, m_BML->Get3dEntityByName("Cam_OrientRef")); // 如果无customcamera
    
    // 设置自定义相机
    m_BML->GetRenderContext()->AttachViewpointToCamera(custom_camera);
    // 获取操作方向四元数
    m_BML->Get3dEntityByName("Cam_OrientRef")->GetQuaternion(&quaternion_Cam_OrientRef);
    // 获取球坐标
    m_BML->Get3dEntityByName("Cam_OrientRef")->GetPosition(&position_Cam_OrientRef);
    // 根据球坐标和操作方向四元数计算新相机位置坐标
    position_custom_camera = CalculateCameraPosition(position_Cam_OrientRef, quaternion_Cam_OrientRef, camera_angle_right);
    quaternion_custom_camera = CalculateCameraQuaternion(quaternion_Cam_OrientRef, camera_angle_right);

    // 设置新相机
    custom_camera->SetFov(DEFAULT_FOV* camera_fov);
    custom_camera->SetPosition(&position_custom_camera);
    custom_camera->SetQuaternion(&quaternion_custom_camera);
    // 数据显示
    if (showdata_enabled){
        OnDrawInfo();
    }
}

void ControlDirCamera::OnDrawInfo() {
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
        VxQuaternion m_OriginalRot_info_1;
        m_BML->Get3dEntityByName("Cam_OrientRef")->GetQuaternion(&m_OriginalRot_info_1);
        ImGui::Text("Ball Control Orientation\n%s", VxQuaternionToString(m_OriginalRot_info_1).c_str());

        VxVector m_OriginalRot_info_2;
        m_BML->Get3dEntityByName("Cam_Target")->GetPosition(&m_OriginalRot_info_2);
        ImGui::Text("Ball Position\n%s", VxVectorToString(m_OriginalRot_info_2).c_str());

        VxQuaternion m_OriginalRot_info_3;
        m_BML->GetTargetCameraByName("InGameCam")->GetQuaternion(&m_OriginalRot_info_3);
        ImGui::Text("OriginalCam Orientation\n%s", VxQuaternionToString(m_OriginalRot_info_3).c_str());
        
        VxVector m_OriginalRot_info_4;
        m_BML->GetTargetCameraByName("InGameCam")->GetPosition(&m_OriginalRot_info_4);
        ImGui::Text("OriginalCam Position\n%s", VxVectorToString(m_OriginalRot_info_4).c_str());
        
        ImGui::Text("ControlDirCam Position\n%s", VxVectorToString(position_custom_camera).c_str());
        ImGui::Text("ControlDirCam Orientation\n%s", VxQuaternionToString(quaternion_custom_camera).c_str());
    }
    ImGui::End();
}
