#include "RainMode.h"
using namespace ScriptHelper;


IMod* BMLEntry(IBML* bml) {
    return new RainMode(bml);
}

void RainMode::OnLoad() {
    mod_enabled_config = GetConfig()->GetProperty("Main", "Enabled");
    mod_enabled_config->SetDefaultBoolean(true);
    mod_enabled_config->SetComment("Enabled to use this mod.");
    
    generate_time_interval_config = GetConfig()->GetProperty("Main", "Time_Interval");
    generate_time_interval_config->SetComment("Time interval for waves of items generated. \nDefault: 150.0 (milliseconds)");
    generate_time_interval_config->SetDefaultFloat(150.0f);
   
    entities_per_wave_config = GetConfig()->GetProperty("Main", "Entities_Interval");
    entities_per_wave_config->SetComment("How many entities will be generate per interval. \nDefault: 10");
    entities_per_wave_config->SetDefaultInteger(10);

    rain_region_config = GetConfig()->GetProperty("Main", "Rain_Region");
    rain_region_config->SetComment("The spatial extent size of generating entities.\nDefault: 80");
    rain_region_config->SetDefaultFloat(80.0f);

    raindrop_generation_height_config = GetConfig()->GetProperty("Main", "Rain_Height");
    raindrop_generation_height_config->SetComment("The generation height of the entities. \nDefault: 60");
    raindrop_generation_height_config->SetDefaultFloat(60.0f);

    rain_region_ball_speed_correlation_config = GetConfig()->GetProperty("Main", "Ball_Speed_Correlation");
    rain_region_ball_speed_correlation_config->SetComment("How ball's speed affect rain-covered areas. \nDefault: 1.0 \nvalue range: 0-N/A");
    rain_region_ball_speed_correlation_config->SetDefaultFloat(1.0f);

    raindrop_intensity_state_config = GetConfig()->GetProperty("Main", "Intensity_State");
    raindrop_intensity_state_config->SetComment("The magnitude of the initial impulse applied to the entities. \nDefault: 1.0 \nvalue range: 0-N/A\nIt is recommended not to exceed 2");
    raindrop_intensity_state_config->SetDefaultFloat(1.0f);

    max_TempBalls_config = GetConfig()->GetProperty("Main", "Entities_Capacity");
    max_TempBalls_config->SetComment("The earliest generated entity will be cleared when capacity is exceeded. \nDefault: 4000");
    max_TempBalls_config->SetDefaultInteger(4000);

    entities_proportion_config = GetConfig()->GetProperty("Main", "Entities_Proportion");
    entities_proportion_config->SetComment("Proportion of generated entities. \n\"paper:wood:stone:box\" \nDefault: 1:1:1:1\ndont use chinese colon");
    entities_proportion_config->SetDefaultString("1:1:1:1");


    m_Balls[0] = (CK3dEntity*)ExecuteBB::ObjectLoad("3D Entities\\PH\\P_Ball_Paper.nmo", true, "P_Ball_Paper_MF").second;
    m_Balls[1] = (CK3dEntity*)ExecuteBB::ObjectLoad("3D Entities\\PH\\P_Ball_Wood.nmo", true, "P_Ball_Wood_MF").second;
    m_Balls[2] = (CK3dEntity*)ExecuteBB::ObjectLoad("3D Entities\\PH\\P_Ball_Stone.nmo", true, "P_Ball_Stone_MF").second;
    m_Balls[3] = (CK3dEntity*)ExecuteBB::ObjectLoad("3D Entities\\PH\\P_Box.nmo", true, "P_Box_MF").second;
    
    load_config();
}
// copied items
void RainMode::OnLoadObject(const char* filename, CKBOOL isMap, const char* masterName, CK_CLASSID filterClass, CKBOOL addToScene, CKBOOL reuseMeshes,
    CKBOOL reuseMaterials, CKBOOL dynamic, XObjectArray* objArray, CKObject* masterObj) {
    if (!strcmp(filename, "3D Entities\\Camera.nmo")) {
        m_CamOrientRef = m_BML->Get3dEntityByName("Cam_OrientRef");
        m_CamTarget = m_BML->Get3dEntityByName("Cam_Target");
    }
}
void RainMode::OnLoadScript(const char* filename, CKBehavior* script) {
    if (!strcmp(script->GetName(), "Gameplay_Ingame"))
        OnEditScript_Gameplay_Ingame(script);

    if (!strcmp(script->GetName(), "Gameplay_Events"))
        OnEditScript_Gameplay_Events(script);
}
void RainMode::OnEditScript_Gameplay_Ingame(CKBehavior* script) {
    // GetLogger()->Info("Debug Ball Force");
    CKBehavior* ballNav = FindFirstBB(script, "Ball Navigation");
    CKBehavior* nop[2] = { nullptr, nullptr };
    FindBB(ballNav, [&nop](CKBehavior* beh) {
        if (nop[0]) nop[1] = beh;
        else nop[0] = beh;
        return !nop[1];
        }, "Nop");
    CKBehavior* keyevent[2] = { CreateBB(ballNav, VT_CONTROLLERS_KEYEVENT), CreateBB(ballNav, VT_CONTROLLERS_KEYEVENT) };
    m_BallForce[0] = CreateParamValue(ballNav, "Up", CKPGUID_KEY, CKKEYBOARD(0));
    m_BallForce[1] = CreateParamValue(ballNav, "Down", CKPGUID_KEY, CKKEYBOARD(0));
    CKBehavior* phyforce[2] = { CreateBB(ballNav, PHYSICS_RT_PHYSICSFORCE, true),
                               CreateBB(ballNav, PHYSICS_RT_PHYSICSFORCE, true) };
    CKBehavior* op = FindFirstBB(ballNav, "Op");
    CKParameter* mass = op->GetInputParameter(0)->GetDirectSource();
    CKBehavior* spf = FindFirstBB(ballNav, "SetPhysicsForce");
    CKParameter* dir[2] = { CreateParamValue(ballNav, "Up", CKPGUID_VECTOR, VxVector(0, 1, 0)),
                           CreateParamValue(ballNav, "Down", CKPGUID_VECTOR, VxVector(0, -1, 0)) };
    CKBehavior* wake = FindFirstBB(ballNav, "Physics WakeUp");

    for (int i = 0; i < 2; i++) {
        keyevent[i]->GetInputParameter(0)->SetDirectSource(m_BallForce[i]);
        CreateLink(ballNav, nop[0], keyevent[i], 0, 0);
        CreateLink(ballNav, nop[1], keyevent[i], 0, 1);
        phyforce[i]->GetTargetParameter()->ShareSourceWith(spf->GetTargetParameter());
        phyforce[i]->GetInputParameter(0)->ShareSourceWith(spf->GetInputParameter(0));
        phyforce[i]->GetInputParameter(1)->ShareSourceWith(spf->GetInputParameter(1));
        phyforce[i]->GetInputParameter(2)->SetDirectSource(dir[i]);
        phyforce[i]->GetInputParameter(3)->ShareSourceWith(spf->GetInputParameter(3));
        phyforce[i]->GetInputParameter(4)->SetDirectSource(mass);
        CreateLink(ballNav, keyevent[i], phyforce[i], 0, 0);
        CreateLink(ballNav, keyevent[i], phyforce[i], 1, 1);
        CreateLink(ballNav, nop[1], phyforce[i], 0, 1);
        CreateLink(ballNav, phyforce[i], wake, 0, 0);
        CreateLink(ballNav, phyforce[i], wake, 1, 0);
    }

    CKBehavior* ballMgr = FindFirstBB(script, "BallManager");
    m_DynamicPos = FindNextBB(script, ballMgr, "TT Set Dynamic Position");

    CKBehavior* newBall = FindFirstBB(ballMgr, "New Ball");
    m_PhysicsNewBall = FindFirstBB(newBall, "physicalize new Ball");

    CKBehavior* trafoMgr = FindFirstBB(script, "Trafo Manager");
    m_SetNewBall = FindFirstBB(trafoMgr, "set new Ball");
    CKBehavior* sop = FindFirstBB(m_SetNewBall, "Switch On Parameter");
    m_CurTrafo = sop->GetInputParameter(0)->GetDirectSource();
    m_CurLevel = m_BML->GetArrayByName("CurrentLevel");
    m_IngameParam = m_BML->GetArrayByName("IngameParameter");
}
void RainMode::OnEditScript_Gameplay_Events(CKBehavior* script) {
    CKBehavior* id = FindNextBB(script, script->GetInput(0));
    m_CurSector = id->GetOutputParameter(0)->GetDestination(0);
}


void RainMode::OnPostResetLevel() {
    CKDataArray* ph = m_BML->GetArrayByName("PH");
    for (auto iter = m_TempBalls.rbegin(); iter != m_TempBalls.rend(); iter++) {
        ph->RemoveRow(iter->first);
        m_BML->GetCKContext()->DestroyObject(iter->second);
    }
    m_TempBalls.clear();
}

RainMode::Camera_Info_Struct RainMode::Get_Camera_Info(){
    Camera_Info_Struct cur_camera_info;
    // time stamp
    cur_camera_info.time_stamp = m_BML->GetTimeManager()->GetTime();
    // vector
    camera_matrix = m_BML->Get3dEntityByName("Cam_OrientRef")->GetWorldMatrix();
    const float proj_x = camera_matrix[2][0];
    const float proj_z = camera_matrix[2][2];
    float inv_length = 0.f;
    const float sq_len = proj_x * proj_x + proj_z * proj_z;
    if (sq_len > 1e-10f) {
        inv_length = 1.0f / std::sqrt(sq_len);
    }
    
    if (inv_length > 0) {
        cur_camera_info.camera_vector_x = proj_x * inv_length;
        cur_camera_info.camera_vector_z = proj_z * inv_length;
    }
    // position
    cur_camera_info.camera_position_x = camera_matrix[3][0];
    cur_camera_info.camera_position_y = camera_matrix[3][1];
    cur_camera_info.camera_position_z = camera_matrix[3][2];
    // speed

    float delta_time = cur_camera_info.time_stamp - last_camera_info.time_stamp;

    float dx = (cur_camera_info.camera_position_x - last_camera_info.camera_position_x);
    float dy = (cur_camera_info.camera_position_y - last_camera_info.camera_position_y);
    float dz = (cur_camera_info.camera_position_z - last_camera_info.camera_position_z);

    cur_camera_info.ball_speed_vector_x = (dx / delta_time) * 1000.0f;
    cur_camera_info.ball_speed_vector_y = (dy / delta_time) * 1000.0f;
    cur_camera_info.ball_speed_vector_z = (dz / delta_time) * 1000.0f;

    //
    last_camera_info = cur_camera_info;

    return cur_camera_info;
}

std::vector<VxVector> RainMode::Random_Positions_Generator(auto &cur_camera_info) {
    // 先确定随机数范围，再根据随机数范围和entities_per_wave生成坐标集
    // 随机数范围
    /// 先处理xz平面位置
    float centerX, centerZ, centerY; //绝对坐标
    centerX = cur_camera_info.camera_position_x //基础坐标
        + ENTITY_FALL_AVG_TIME * cur_camera_info.ball_speed_vector_x * region_speed_correlation // 速度偏移
        + cur_camera_info.camera_vector_x * CAMERA_VECTOR_OFFSET * rain_region; // 视角偏移
    centerZ = cur_camera_info.camera_position_z 
        + ENTITY_FALL_AVG_TIME * cur_camera_info.ball_speed_vector_z * region_speed_correlation
        + cur_camera_info.camera_vector_z * CAMERA_VECTOR_OFFSET * rain_region;
    //// 计算 center Y 生成高度
    float centerY_speed_offset = ENTITY_FALL_AVG_TIME * cur_camera_info.ball_speed_vector_x * region_speed_correlation;
    centerY_speed_offset= std::max(0.0f, centerY_speed_offset);
    centerY = cur_camera_info.camera_position_y //基础坐标
        + raindrop_generation_height //基础高度偏移
        + centerY_speed_offset; // 速度偏移    
    std::vector<VxVector> random_positions{};
    random_positions.reserve(entities_per_wave);

    // 2. 设置随机数生成器
    std::random_device rd;
    std::mt19937 gen(rd());

    // 3. 定义分布范围 [center - half, center + half]
    float half_side = rain_region / 2.0f;
    std::uniform_real_distribution<float> distX(centerX - half_side, centerX + half_side);
    std::uniform_real_distribution<float> distZ(centerZ - half_side, centerZ + half_side);

    // 4. 生成坐标
    for (int i = 0; i < entities_per_wave; ++i) {
        float randomX = distX(gen);
        float randomZ = distZ(gen);

        // y 坐标保持固定（因为平面平行于 xz 平面）
        random_positions.emplace_back(randomX, centerY, randomZ);
    }
    return random_positions;
}

void RainMode::Raindrop_Generator(const std::vector<VxVector>& positions) {
    if (positions.empty()) return;
    // 先确定物件类型比例
    int num_paper, num_wood, num_stone, num_box;
    sscanf_s(entities_proportion, "%d:%d:%d:%d", &num_paper, &num_wood, &num_stone, &num_box);
    float sum = (float)(num_paper + num_wood + num_stone + num_box);
    float ratio_paper = (float)num_paper / sum;
    float ratio_wood = (float)num_wood / sum;
    float ratio_stone = (float)num_stone / sum;
    float ratio_box = (float)num_box / sum;
    int raindrop_type;
    std::random_device rd;
    std::mt19937 gen(rd());
    std::discrete_distribution<> dist({ ratio_paper, ratio_wood, ratio_stone, ratio_box });
    CKContext* context = m_BML->GetCKContext();
    CKGroup* depth = m_BML->GetGroupByName("DepthTest");
    for (const auto& pos : positions) {
        // 生成随机类型
        raindrop_type = dist(gen);
        // 生成实体
        m_CurObj = (CK3dEntity*)m_BML->GetCKContext()->CopyObject(m_Balls[raindrop_type]);
        m_CurObj->SetPosition(&pos);
        m_CurObj->Show();
        CKMesh* mesh = m_CurObj->GetMesh(0);
        switch (raindrop_type) {
        case 0:
            ExecuteBB::PhysicalizeConvex(m_CurObj, false, 0.5f, 0.4f, 0.2f, "", false, true, false, 1.5f, 0.1f, mesh->GetName(), VxVector(0, 0, 0), mesh);
            break;
        case 1:
            ExecuteBB::PhysicalizeBall(m_CurObj, false, 0.6f, 0.2f, 2.0f, "", false, true, false, 0.6f, 0.1f, mesh->GetName());
            break;
        case 2:
            ExecuteBB::PhysicalizeBall(m_CurObj, false, 0.7f, 0.1f, 10.0f, "", false, true, false, 0.2f, 0.1f, mesh->GetName());
            break;
        default:
            ExecuteBB::PhysicalizeConvex(m_CurObj, false, 0.7f, 0.3f, 1.0f, "", false, true, false, 0.1f, 0.1f, mesh->GetName(), VxVector(0, 0, 0), mesh);
            break;
        }

        // 1. 定义辅助随机函数 (范围 [min, max])
        auto GetRand = [](float min, float max) {
            return min + static_cast<float>(rand()) / (static_cast<float>(RAND_MAX / (max - min)));
            };

        // --- 参数 2: 相对位置 x,y,z 均为 [-1, 1] ---
        VxVector randomPos(GetRand(-0.7f, 0.7f), GetRand(-0.7f, 0.7f), GetRand(-0.7f, 0.7f));

        // --- 参数 4: 方向，约束 y 范围 [-1, 0] ---
        VxVector randomImpulseDir;
        do {
            randomImpulseDir.Set(GetRand(-1.0f, 1.0f), GetRand(-0.2f, 0.0f), GetRand(-1.0f, 1.0f));
        } while (randomImpulseDir.SquareMagnitude() < 0.001f); // 防止零向量导致除以0
        randomImpulseDir.Normalize(); // 确保平方和开根等于1

        // --- 参数 6: 根据 raindrop_type 计算冲量大小 ---
        float quality = 1.0f;
        switch (raindrop_type) {
        case 0: quality = 1.0f;  break; // paper default 0.2
        case 1: quality = 2.0f;  break; // wood
        case 2: quality = 4.0f; break; // stone default 10
        case 3: quality = 1.0f;  break; // box
        default: quality = 1.0f;
        }

        float randomMagnitude = quality * GetRand(60.0f, 100.0f) * raindrop_intensity_state;

        // --- 执行施加冲量的函数 ---
        // 物件，位置，位置参考原点，方向向量，方向参考原点，冲量大小
        ExecuteBB::PhysicsImpulse(
            m_CurObj,
            randomPos,
            m_CurObj,
            //VxVector(0.0f, 0.0f, 1.0f),
            randomImpulseDir,
            nullptr,
            randomMagnitude
        );


        CKDataArray* ph = m_BML->GetArrayByName("PH");
        // 删除多余m_TempBalls元素避免卡顿
        while (m_TempBalls.size() >= (size_t)max_TempBalls) {
            auto oldest = m_TempBalls.front();
            if (ph) {
                // 手动遍历 PH 表找到对应的行索引，因为对象指针是最可靠的键
                int rowCount = ph->GetRowCount();
                for (int i = 0; i < rowCount; ++i) {
                    CKObject* obj = ph->GetElementObject(i, 3); // 第3列是存放 m_CurObj 的
                    if (obj == (CKObject*)oldest.second) {
                        ph->RemoveRow(i);
                        break; // 找到并删除后立即跳出
                    }
                }
            }

            // 2. 销毁场景中的实体物件
            if (oldest.second) {
                ExecuteBB::Unphysicalize(oldest.second);
                m_BML->GetCKContext()->DestroyObject(oldest.second);
            }

            // 3. 从队列中弹出
            m_TempBalls.pop_front();
        }
        ph->AddRow();
        int index = ph->GetRowCount() - 1;
        ph->SetElementValueFromParameter(index, 0, m_CurSector);
        static char P_BALL_NAMES[4][13] = { "P_Ball_Paper", "P_Ball_Wood", "P_Ball_Stone", "P_Box" };
        ph->SetElementStringValue(index, 1, P_BALL_NAMES[raindrop_type]);
        VxMatrix matrix = m_CurObj->GetWorldMatrix();
        ph->SetElementValue(index, 2, &matrix);
        ph->SetElementObject(index, 3, m_CurObj);
        CKBOOL set = false;
        ph->SetElementValue(index, 4, &set);

        CKGroup* depth = m_BML->GetGroupByName("DepthTest");
        depth->AddObject(m_CurObj);
        m_TempBalls.emplace_back(index, m_CurObj);
        m_CurObj = nullptr;
    }
    m_InputHook->SetBlock(false);
}

void RainMode::OnProcess() {
    if (!(mod_enabled && m_BML->IsPlaying()))
        return;
    // 确定是否在在interval后，并不是每帧都要生成，节省性能

    bool pos_update = false;
    float current_time = m_BML->GetTimeManager()->GetTime();
    float time_diff = current_time - next_pos_update_time;
    if (time_diff >= 0) {
        pos_update = true;
        if (time_diff > 3 * generate_time_interval){
            next_pos_update_time = current_time; }
        else {
            next_pos_update_time += generate_time_interval;}
    }
    // test
    // if (m_InputHook->IsKeyPressed(CKKEY_M)) {
    //     m_BML->SendIngameMessage(("data: "
    //         + std::to_string(last_camera_info.camera_vector_x) + "\n"
    //         + std::to_string(last_camera_info.camera_position_x) + "\n"
    //         + std::to_string(last_camera_info.ball_speed_vector_x) + "\n"
    //         + std::to_string(last_camera_info.time_stamp) + "\n"
    //         ).c_str());
    // 
    // }
    // *test
    if (!pos_update) {
        return;
    }
    // 获取所有必要数据：当前位置坐标，视角向量，球速向量
    auto cur_camera_info = Get_Camera_Info();
    std::vector<VxVector> random_positions = Random_Positions_Generator(cur_camera_info);
    Raindrop_Generator(random_positions);

}