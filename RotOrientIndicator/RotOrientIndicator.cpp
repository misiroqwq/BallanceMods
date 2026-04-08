#include "RotOrientIndicator.h"

IMod* BMLEntry(IBML* bml) {
    return new RotOrientIndicator(bml);
}

void RotOrientIndicator::OnLoad() {

    mod_enabled_config = GetConfig()->GetProperty("Main", "Enabled");
    mod_enabled_config->SetDefaultBoolean(true);
    mod_enabled_config->SetComment("Enabled to use this mod.");

    show_info_enabled_config = GetConfig()->GetProperty("Main", "Show_Data");
    show_info_enabled_config->SetDefaultBoolean(true);
    show_info_enabled_config->SetComment("Enable display of rotation axis vector and speed.");

    rotation_effect_enabled_config = GetConfig()->GetProperty("Main", "Rotation_Effect");
    rotation_effect_enabled_config->SetDefaultBoolean(true);
    rotation_effect_enabled_config->SetComment("Enable sphere rotation direction indicator (ring orientation).");

    scaling_effect_enabled_config = GetConfig()->GetProperty("Main", "Scaling_Effect");
    scaling_effect_enabled_config->SetDefaultBoolean(true);
    scaling_effect_enabled_config->SetComment("Enable sphere speed magnitude indicator (ring scaling).");
    
    scaling_effect_max_scale_config = GetConfig()->GetProperty("Main", "Max_Scale");
    scaling_effect_max_scale_config->SetDefaultFloat(2.8f);
    scaling_effect_max_scale_config->SetComment("Maximum scaling factor of the ring (applied when speed reaches Max_Velocity). Default: 2.8");

    scaling_effect_max_velocity_config = GetConfig()->GetProperty("Main", "Max_Velocity");
    scaling_effect_max_velocity_config->SetDefaultFloat(24.0f);
    scaling_effect_max_velocity_config->SetComment("Maximum rotation speed when the ring is at its largest. Default: 24.0");

    load_config();
}
void RotOrientIndicator::OnPostStartMenu() {
    if (init) return;
    current_level_array = m_BML->GetArrayByName("CurrentLevel");
    auto pm = m_BML->GetPathManager();
    XString data_path_index_name = "Data Paths";
    int data_path_index = pm->GetCategoryIndex(data_path_index_name);
    XString indicator_file("Rotation_Indicator.nmo");
    pm->ResolveFileName(indicator_file, data_path_index);
    ExecuteBB::ObjectLoad(indicator_file.CStr(), false);
    indicator = static_cast<CK3dObject*>(m_BML->GetCKContext()->GetObjectByName("___Rotation_Indicator"));
    init = true;
}
float RotOrientIndicator::LinearMap(int input) {
    // 确保输入在有效范围内（0 到 scaling_effect_max_velocity）
    if (input < 0) input = 0;
    if (input > scaling_effect_max_velocity) input = static_cast<int>(scaling_effect_max_velocity);

    // 安全性检查：避免除以0
    if (scaling_effect_max_velocity <= 0) return scaling_effect_max_scale;

    // 线性映射公式
    const float ratio = (scaling_effect_max_scale - 1.0f) / scaling_effect_max_velocity;
    return 1.0f + static_cast<float>(input) * ratio;
}

void RotOrientIndicator::OnProcess() {
    if (!mod_enabled || !m_BML->IsPlaying()) { return; };
    current_ball = static_cast<CK3dObject*>(current_level_array->GetElementObject(0, 1));
    // Position
    VxVector pos;
    current_ball->GetPosition(&pos);
    indicator->SetPosition(&pos);
    // Quaternion
    VxQuaternion CurBallVxQuaternion;
    current_ball->GetQuaternion(&CurBallVxQuaternion);
    CalculateRotationAxis(PreBallVxQuaternion, CurBallVxQuaternion, axisX, axisY, axisZ);
    if (rotation_effect_enabled){
        VxQuaternion indicator_quaternion = CreateQuaternionFromVector(axisX, axisY, axisZ);
        indicator->SetQuaternion(&indicator_quaternion);
    }
    // Velocity
    RotationVelocity = CalculateRotationVelocity(PreBallVxQuaternion, CurBallVxQuaternion);
    if (scaling_effect_enabled) {
        // 将速率(0-max_velocity)映射到缩放(1-max_scale)
        float EntityScale = LinearMap(RotationVelocity);
        indicator->SetScale3f(EntityScale, EntityScale, 1.0f);
    }
    // 保存本帧
    PreBallVxQuaternion = CurBallVxQuaternion;
    //数据显示
    if (show_info_enabled) { OnDrawInfo(); }
}



// 计算旋转轴
void RotOrientIndicator::CalculateRotationAxis(
    const VxQuaternion& prevQuat,
    const VxQuaternion& currQuat,
    float& axisX, float& axisY, float& axisZ
) {
    // 1. 将 VxQuaternion 转换为标准意义上的四元数 (xyz 取反)
    float p_w = prevQuat.w;
    float p_x = -prevQuat.x;
    float p_y = -prevQuat.y;
    float p_z = -prevQuat.z;

    float c_w = currQuat.w;
    float c_x = -currQuat.x;
    float c_y = -currQuat.y;
    float c_z = -currQuat.z;

    // 2. 求 prevQuat 标准四元数的逆 (即共轭四元数，xyz再次取反)
    // 所以逆四元数的 xyz 实际上就等于最初传入的 prevQuat 的 xyz
    float inv_p_w = p_w;
    float inv_p_x = -p_x;
    float inv_p_y = -p_y;
    float inv_p_z = -p_z;

    // 3. 计算相对旋转差量 deltaQ = currQuat * inv(prevQuat)
    // 根据标准四元数乘法公式：q1 * q2
    // w = w1*w2 - x1*x2 - y1*y2 - z1*z2
    // x = w1*x2 + x1*w2 + y1*z2 - z1*y2
    // y = w1*y2 - x1*z2 + y1*w2 + z1*x2
    // z = w1*z2 + x1*y2 - y1*x2 + z1*w2
    float dq_w = c_w * inv_p_w - c_x * inv_p_x - c_y * inv_p_y - c_z * inv_p_z;
    float dq_x = c_w * inv_p_x + c_x * inv_p_w + c_y * inv_p_z - c_z * inv_p_y;
    float dq_y = c_w * inv_p_y - c_x * inv_p_z + c_y * inv_p_w + c_z * inv_p_x;
    float dq_z = c_w * inv_p_z + c_x * inv_p_y - c_y * inv_p_x + c_z * inv_p_w;

    // 4. 确保走最短旋转路径
    // 如果 w < 0，代表旋转角超过 180 度，将四元数取反
    if (dq_w < 0.0f) {
        dq_w = -dq_w;
        dq_x = -dq_x;
        dq_y = -dq_y;
        dq_z = -dq_z;
    }

    // 5. 提取旋转轴并归一化
    float magnitude = std::sqrt(dq_x * dq_x + dq_y * dq_y + dq_z * dq_z);

    // 防止除以0 (即没有发生旋转的情况)
    if (magnitude > 1e-6f) {
        axisX = dq_x / magnitude;
        axisY = dq_y / magnitude;
        axisZ = dq_z / magnitude;
    }
    else {
        // 两帧之间没有旋转，给一个默认的有效旋转轴
        axisX = 0.0f;
        axisY = 0.0f;
        axisZ = 1.0f;
    }
}



VxQuaternion RotOrientIndicator::CreateQuaternionFromVector(float axisX, float axisY, float axisZ) {
    const float tolerance = 1e-6f; // 浮点误差容忍度

    // 计算向量长度并归一化
    float lengthSq = axisX * axisX + axisY * axisY + axisZ * axisZ;
    if (lengthSq < tolerance) {
        // 零向量，返回单位四元数
        return VxQuaternion(0.0f, 0.0f, 0.0f, 1.0f);
    }
    float invLength = 1.0f / std::sqrt(lengthSq);
    float nx = axisX * invLength;
    float ny = axisY * invLength;
    float nz = axisZ * invLength;
    // 参考向量: (0, 0, 1)
    float dot = nz; // (0* nx + 0* ny + 1* nz)
    if (dot > 0.999999f) {
        // 方向接近 (0,0,1)，返回单位四元数
        return VxQuaternion(0.0f, 0.0f, 0.0f, 1.0f);
    }
    else if (dot < -0.999999f) {
        // 方向接近 (0,0,-1)，绕 Y 轴旋转 180°
        return VxQuaternion(0.0f, 1.0f, 0.0f, 0.0f); // w=0, (x,y,z)=(0,1,0)
    }
    // 计算旋转轴: (0,0,1) × (nx,ny,nz) = (-ny, nx, 0)
    float crossX = -ny;
    float crossY = nx;
    float crossZ = 0;
    // 计算 s = 2 * cos(θ/2)^2 = (1 + dot)
    float s = std::sqrt((1.0f + dot) * 2.0f);
    float invS = 1.0f / s;
    // 构造四元数: w = s / 2, (x, y, z) = 旋转轴 * invS
    return VxQuaternion(
        - crossX * invS,
        - crossY * invS,
        - crossZ * invS,
        s * 0.5f
    );
}




float RotOrientIndicator::CalculateRotationVelocity(
    const VxQuaternion& prevQuat,
    const VxQuaternion& currQuat
) {
    float deltaTime = m_BML->GetTimeManager()->GetLastDeltaTime() / 1000.0f;

    // 防止除以0
    if (deltaTime <= 1e-6f) {
        return 0.0f;
    }

    // 1. 计算两个四元数的模长 (防漂移)
    float lenPrev = std::sqrt(prevQuat.w * prevQuat.w + prevQuat.x * prevQuat.x + prevQuat.y * prevQuat.y + prevQuat.z * prevQuat.z);
    float lenCurr = std::sqrt(currQuat.w * currQuat.w + currQuat.x * currQuat.x + currQuat.y * currQuat.y + currQuat.z * currQuat.z);

    // 如果出现奇异值，直接返回0
    if (lenPrev < 1e-6f || lenCurr < 1e-6f) {
        return 0.0f;
    }

    // 2. 计算点乘，并除以模长的乘积，实现归一化点乘
    float dotProduct = (prevQuat.w * currQuat.w +
        prevQuat.x * currQuat.x +
        prevQuat.y * currQuat.y +
        prevQuat.z * currQuat.z) / (lenPrev * lenCurr);

    // 3. 处理最短旋转路径
    if (dotProduct < 0.0f) {
        dotProduct = -dotProduct;
    }

    // 4. 【关键修复】浮点数精度死区 (Deadzone)
    // 如果点乘非常接近 1.0 (例如 0.99999)，说明几乎没有旋转。
    // 直接返回 0，防止 acos 将浮点数底噪放大。
    if (dotProduct > 0.99999999f) {
        return 0.0f;
    }

    // 5. 计算最终角度与角速度
    float angle = 2.0f * std::acos(dotProduct);
    return angle / deltaTime;
}



void RotOrientIndicator::OnDrawInfo() {
    constexpr ImGuiWindowFlags WinFlags = ImGuiWindowFlags_AlwaysAutoResize |
        ImGuiWindowFlags_NoDecoration |
        ImGuiWindowFlags_NoNav |
        ImGuiWindowFlags_NoFocusOnAppearing |
        ImGuiWindowFlags_NoBringToFrontOnFocus |
        ImGuiWindowFlags_NoSavedSettings;
    // 设置窗口在右上角显示
    // 锚点(1.0, 0.0)表示右上角，间距(10,10)提供一些边距
    ImGui::SetNextWindowPos(ImVec2(ImGui::GetIO().DisplaySize.x - 10, 10), ImGuiCond_Always, ImVec2(1.0f, 0.0f));

    if (ImGui::Begin("Camera Info", nullptr, WinFlags)) { //沿用ControlDirCamera的DrawInfo，避免重叠
        ImGui::Text("Rotation axis X, Y, Z \n%.2f, %.2f, %.2f", axisX, axisY, axisZ);
        ImGui::Text("Rotation Velocity \n%.2f", RotationVelocity);
    }
    ImGui::End();
}
