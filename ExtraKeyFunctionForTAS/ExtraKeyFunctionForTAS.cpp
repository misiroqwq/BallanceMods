#include "ExtraKeyFunctionForTAS.h"
using namespace ScriptHelper;

IMod* BMLEntry(IBML* bml) {
    return new ExtraKeyFunctionForTAS(bml);
}

void ExtraKeyFunctionForTAS::OnLoad() {
    mod_enabled_config = GetConfig()->GetProperty("Main", "Enabled");
    mod_enabled_config->SetDefaultBoolean(true);
    mod_enabled_config->SetComment("Enabled to use this mod.");

    TP_info_config = GetConfig()->GetProperty("Main", "TP_info_list");
    TP_info_config->SetComment("(sector, paper|wood|stone, pos_x, pos_y, pos_z), (...), ...");
    TP_info_config->SetDefaultString("(3, paper, 8, 20, -168), (2, wood, 16, 20, -168)"); // (3, paper, 0, 300, 0), (2, wood, 0, 400, 0)

    Summon_info_config = GetConfig()->GetProperty("Main", "Summon_info_list");
    Summon_info_config->SetComment("(paper|wood|stone|box, pos_x, pos_y, pos_z), (...), ...");
    Summon_info_config->SetDefaultString("(paper, 8, 30, -168), (box, 14, 40, -168)"); // (3, paper, 0, 300, 0), (2, wood, 0, 400, 0)

    m_Balls[0] = (CK3dEntity*)ExecuteBB::ObjectLoad("3D Entities\\PH\\P_Ball_Paper.nmo", true, "P_Ball_Paper_MF").second;
    m_Balls[1] = (CK3dEntity*)ExecuteBB::ObjectLoad("3D Entities\\PH\\P_Ball_Wood.nmo", true, "P_Ball_Wood_MF").second;
    m_Balls[2] = (CK3dEntity*)ExecuteBB::ObjectLoad("3D Entities\\PH\\P_Ball_Stone.nmo", true, "P_Ball_Stone_MF").second;
    m_Balls[3] = (CK3dEntity*)ExecuteBB::ObjectLoad("3D Entities\\PH\\P_Box.nmo", true, "P_Box_MF").second;

    load_config();
}

void ExtraKeyFunctionForTAS::OnLoadScript(const char* filename, CKBehavior* script) {
    if (strcmp(script->GetName(), "Gameplay_Ingame") == 0) {
        CKBehavior* ballMgr = ScriptHelper::FindFirstBB(script, "BallManager");
        CKBehavior* newBall = ScriptHelper::FindFirstBB(ballMgr, "New Ball");
        m_dynamicPos = ScriptHelper::FindNextBB(script, ballMgr, "TT Set Dynamic Position");
        m_phyNewBall = ScriptHelper::FindFirstBB(newBall, "physicalize new Ball");
    }
    if (!strcmp(script->GetName(), "Gameplay_Events"))
        OnEditScript_Gameplay_Events(script);
}
void ExtraKeyFunctionForTAS::OnPreStartMenu() {
    if (init_prestartmenu) { return; }
    const auto mod_size = m_BML->GetModCount();
    for (int i = 0; i < mod_size; ++i) {
        auto mod = m_BML->GetMod(i);
        if (std::strcmp(mod->GetID(), "BallanceMMOClient") == 0)
            bmmo_installed = true;
    }
    init_prestartmenu = true;
}

void ExtraKeyFunctionForTAS::OnPostStartMenu() {
    if (init)
        return;

    if (!m_curLevel) {
        m_curLevel = m_BML->GetArrayByName("CurrentLevel");
        m_checkpoints = m_BML->GetArrayByName("Checkpoints");
        m_resetpoints = m_BML->GetArrayByName("ResetPoints");
        m_ingameParam = m_BML->GetArrayByName("IngameParameter");
        CKBehavior* events = m_BML->GetScriptByName("Gameplay_Events");
        CKBehavior* id = ScriptHelper::FindNextBB(events, events->GetInput(0));
        m_curSector = id->GetOutputParameter(0)->GetDestination(0);
    }
    auto* script = m_BML->GetScriptByName("Gameplay_Ingame");
    CKBehavior* trafoMgr = ScriptHelper::FindFirstBB(script, "Trafo Manager");
    m_setNewBall = ScriptHelper::FindFirstBB(trafoMgr, "set new Ball");
    CKBehavior* sop = ScriptHelper::FindFirstBB(m_setNewBall, "Switch On Parameter");
    m_curTrafo = sop->GetInputParameter(0)->GetDirectSource();

    init = true;
}

void ExtraKeyFunctionForTAS::OnPostResetLevel() {
    CKDataArray* ph = m_BML->GetArrayByName("PH");
    for (auto iter = m_TempBalls.rbegin(); iter != m_TempBalls.rend(); iter++) {
        ph->RemoveRow(iter->first);
        m_BML->GetCKContext()->DestroyObject(iter->second);
    }
    m_TempBalls.clear();
}

void ExtraKeyFunctionForTAS::OnEditScript_Gameplay_Events(CKBehavior* script) {
    CKBehavior* id = FindNextBB(script, script->GetInput(0));
    m_CurSector = id->GetOutputParameter(0)->GetDestination(0);
}


void ExtraKeyFunctionForTAS::OnProcess() {
    if (!mod_enabled || bmmo_installed) { return; }
    OnDrawInfo();
    if (m_BML->GetInputManager()->IsKeyDown(CKKEY_Q) && Ball_Active) {
        BallUp();
    }
    if (m_BML->GetInputManager()->IsKeyPressed(CKKEY_SPACE) && Ball_Active) {
        if (!Parse_TP_Info.empty()){
            if (CurrentIndex_TP + 1 < static_cast<int>(Parse_TP_Info.size())) {
                //到达末尾后不再继续
                CurrentIndex_TP++;
                TPWithSpace();
            }
        }
    }
    if (m_BML->GetInputManager()->IsKeyPressed(CKKEY_RETURN) && m_BML->IsPlaying()) {
        if (!Parse_TP_Info.empty()) {
            if (CurrentIndex_Summon + 1 < static_cast<int>(Parse_Summon_Info.size())) {
                CurrentIndex_Summon++;
                SummonWithEnter();
            }
        }
    }
    // /TPwithSpace 保存当前帧数据，下一帧用
    CK3dEntity* camRef = m_BML->Get3dEntityByName("Cam_OrientRef");
    matrix = camRef->GetWorldMatrix();
    for (int i = 0; i < 4; i++) {
        std::swap(matrix[0][i], matrix[2][i]);
        matrix[0][i] = -matrix[0][i];
    }
}
void ExtraKeyFunctionForTAS::SummonWithEnter() {
    const auto& info = Parse_Summon_Info[CurrentIndex_Summon];

    // 2. 将字符串转换为索引 (paper|wood|stone|box)
    int raindrop_type = 0;
    if (info.Summon_balltype == "wood")      raindrop_type = 1;
    else if (info.Summon_balltype == "stone") raindrop_type = 2;
    else if (info.Summon_balltype == "box")   raindrop_type = 3;

    // 3. 准备坐标
    VxVector pos(info.Summon_position[0], info.Summon_position[1], info.Summon_position[2]);

    // 4. 生成实体
    CKContext* context = m_BML->GetCKContext();
    CK3dEntity* m_CurObj = (CK3dEntity*)context->CopyObject(m_Balls[raindrop_type]);
    if (!m_CurObj) return;

    m_CurObj->SetPosition(&pos);
    m_CurObj->Show();
    CKMesh* mesh = m_CurObj->GetMesh(0);

    // 5. 根据类型进行物理化 (复用原函数参数)
    switch (raindrop_type) {
    case 0: // paper
        ExecuteBB::PhysicalizeConvex(m_CurObj, false, 0.5f, 0.4f, 0.2f, "", false, true, false, 1.5f, 0.1f, mesh->GetName(), VxVector(0, 0, 0), mesh);
        break;
    case 1: // wood
        ExecuteBB::PhysicalizeBall(m_CurObj, false, 0.6f, 0.2f, 2.0f, "", false, true, false, 0.6f, 0.1f, mesh->GetName());
        break;
    case 2: // stone
        ExecuteBB::PhysicalizeBall(m_CurObj, false, 0.7f, 0.1f, 10.0f, "", false, true, false, 0.2f, 0.1f, mesh->GetName());
        break;
    default: // box
        ExecuteBB::PhysicalizeConvex(m_CurObj, false, 0.7f, 0.3f, 1.0f, "", false, true, false, 0.1f, 0.1f, mesh->GetName(), VxVector(0, 0, 0), mesh);
        break;
    }

    // 7. 内存管理：清理旧球，维护 PH 表 (防止数量过多卡顿)
    CKDataArray* ph = m_BML->GetArrayByName("PH");
    // 8. 注册新球到 PH 表和 DepthTest 组
    if (ph) {
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
        if (depth) depth->AddObject(m_CurObj);

        m_TempBalls.emplace_back(index, m_CurObj);
    }

    m_CurObj = nullptr;
}
void ExtraKeyFunctionForTAS::TPWithSpace() {
    // 改变小节，如果小节不是当前小节

    //[transport] Get the sector at the time of /nsp
    int target_sector = Parse_TP_Info[CurrentIndex_TP].TP_sector;
    //[transport] Get the current sector
    CKBehavior* events = m_BML->GetScriptByName("Gameplay_Events");
    int cur_sector = ScriptHelper::GetParamValue<int>(ScriptHelper::FindNextBB(events, events->GetInput(0))->GetOutputParameter(0)->GetDestination(0));
    CKContext* ctx = m_BML->GetCKContext();
    if (m_curLevel) {
        if (target_sector != cur_sector) {
            VxMatrix matrix;
            m_resetpoints->GetElementValue(target_sector - 1, 0, &matrix);
            m_curLevel->SetElementValue(0, 3, &matrix);

            m_ingameParam->SetElementValue(0, 1, &target_sector);
            m_ingameParam->SetElementValue(0, 2, &cur_sector);
            ScriptHelper::SetParamValue(m_curSector, target_sector);

            CKBehavior* sectorMgr = m_BML->GetScriptByName("Gameplay_SectorManager");
            ctx->GetCurrentScene()->Activate(sectorMgr, true);

            m_BML->AddTimerLoop(1ul, [this, target_sector, sectorMgr, ctx]() {
                if (sectorMgr->IsActive())
                    return true;

                m_BML->AddTimer(2ul, [this, target_sector, ctx]() {
                    CKBOOL active = false;
                    m_curLevel->SetElementValue(0, 4, &active);

                    CK_ID flameId;
                    m_checkpoints->GetElementValue(target_sector % 2, 1, &flameId);
                    auto* flame = static_cast<CK3dEntity*>(ctx->GetObject(flameId));
                    ctx->GetCurrentScene()->Activate(flame->GetScript(0), true);

                    m_checkpoints->GetElementValue(target_sector - 1, 1, &flameId);
                    flame = static_cast<CK3dEntity*>(ctx->GetObject(flameId));
                    ctx->GetCurrentScene()->Activate(flame->GetScript(0), true);

                    if (target_sector > m_checkpoints->GetRowCount()) {
                        CKMessageManager* mm = m_BML->GetMessageManager();
                        CKMessageType msg = mm->AddMessageType("last Checkpoint reached");
                        mm->SendMessageSingle(msg, m_BML->GetGroupByName("All_Sound"));
                    }
                    else {
                        m_BML->AddTimer(2ul, [this, target_sector, ctx, flame]() {
                            VxMatrix matrix;
                            m_checkpoints->GetElementValue(target_sector - 1, 0, &matrix);
                            flame->SetWorldMatrix(matrix);
                            CKBOOL active = true;
                            m_curLevel->SetElementValue(0, 4, &active);
                            ctx->GetCurrentScene()->Activate(flame->GetScript(0), true);
                            m_BML->Show(flame, CKSHOW, true);
                            });
                        }
                    });
                return false;
            });
        }
    }

    //[transport] Ball position transport
    CKMessageManager* mm = m_BML->GetMessageManager();
    CKMessageType ballDeact = mm->AddMessageType("BallNav deactivate");
    mm->SendMessageSingle(ballDeact, m_BML->GetGroupByName("All_Gameplay"));
    mm->SendMessageSingle(ballDeact, m_BML->GetGroupByName("All_Sound"));
    m_dynamicPos->ActivateInput(1);
    m_dynamicPos->Activate();
    auto current_ball = static_cast<CK3dEntity*>(m_curLevel->GetElementObject(0, 1));

    m_BML->AddTimer(2ul, [this, current_ball, mm, ballDeact]() {
        ExecuteBB::Unphysicalize(current_ball);



        CK3dEntity* camMF = m_BML->Get3dEntityByName("Cam_MF");


        VxVector tmp_Vx_Positon(
            Parse_TP_Info[CurrentIndex_TP].TP_position[0], 
            Parse_TP_Info[CurrentIndex_TP].TP_position[1], 
            Parse_TP_Info[CurrentIndex_TP].TP_position[2]);
        // current_ball->SetWorldMatrix(matrix);
        current_ball->SetPosition(&tmp_Vx_Positon);
        m_BML->RestoreIC(camMF, true);
        matrix[3][0] = Parse_TP_Info[CurrentIndex_TP].TP_position[0];
        matrix[3][1] = Parse_TP_Info[CurrentIndex_TP].TP_position[1];
        matrix[3][2] = Parse_TP_Info[CurrentIndex_TP].TP_position[2];
        camMF->SetWorldMatrix(matrix);
        // camMF->SetPosition(&p);



        m_dynamicPos->ActivateInput(0);
        m_dynamicPos->Activate();
        m_phyNewBall->ActivateInput(0);
        m_phyNewBall->Activate();
        m_phyNewBall->GetParent()->Activate();
        mm->SendMessageSingle(ballDeact, m_BML->GetGroupByName("All_Gameplay"));
        mm->SendMessageSingle(ballDeact, m_BML->GetGroupByName("All_Sound"));

        //[transport] Ball type reset
        CK3dEntity* curBall = static_cast<CK3dEntity*>(m_curLevel->GetElementObject(0, 1));
        ExecuteBB::Unphysicalize(curBall);
        // static char trafoTypes[3][6] = { "paper", "wood", "stone" };
        char* tmp_ball_type = strdup(Parse_TP_Info[CurrentIndex_TP].TP_balltype.c_str());
        SetParamString(m_curTrafo, tmp_ball_type);
        free(tmp_ball_type);
        m_setNewBall->ActivateInput(0);
        m_setNewBall->Activate();
    });
    m_BML->SendIngameMessage(("ExtraKeyFunctionForTAS TP: Sector "
        + std::to_string(target_sector) + ", position "
        + std::to_string(Parse_TP_Info[CurrentIndex_TP].TP_position[0]) + ", "
        + std::to_string(Parse_TP_Info[CurrentIndex_TP].TP_position[1]) + ", "
        + std::to_string(Parse_TP_Info[CurrentIndex_TP].TP_position[2]) + ")").c_str());
}

void ExtraKeyFunctionForTAS::BallUp() {
    CKDataArray* current_level_array_ = m_BML->GetArrayByName("CurrentLevel");
    CK3dEntity* currentBall = static_cast<CK3dObject*>((current_level_array_)->GetElementObject(0, 1));

    std::unordered_map<std::string, float> ball_name_to_quality{ {"Ball_Paper", 0.2f }, {"Ball_Wood", 1.9f}, {"Ball_Stone", 10.0f} }; // 0.2 | 1.9 | 10
    float ball_quality = ball_name_to_quality[currentBall->GetName()];

    // 参数说明: 目标实体, 施力点(默认0,0,0代表球心), 施力点参考系, 方向, 方向参考系, 冲量大小
    ExecuteBB::PhysicsImpulse(currentBall, VxVector(0, 0, 0), currentBall, VxVector(0.0f, 1.0f, 0.0f), nullptr, ball_quality*0.9f);
    ExecuteBB::PhysicsWakeUp(currentBall);
}

void ExtraKeyFunctionForTAS::OnDrawInfo() {
    constexpr ImGuiWindowFlags WinFlags = ImGuiWindowFlags_AlwaysAutoResize |
        ImGuiWindowFlags_NoDecoration |
        ImGuiWindowFlags_NoNav |
        ImGuiWindowFlags_NoFocusOnAppearing |
        ImGuiWindowFlags_NoBringToFrontOnFocus |
        ImGuiWindowFlags_NoSavedSettings;

    ImGui::SetNextWindowPos(ImVec2(10, 20), ImGuiCond_Always, ImVec2(0.0f, 0.0f));
    if (ImGui::Begin("CheatFunctionEnabled", nullptr, WinFlags)) {
        VxQuaternion m_OriginalRot_info_1;
        ImGui::Text("CheatFunctionEnabled");

        // VxQuaternion m_OriginalRot_info_2;
        // m_BML->Get3dEntityByName("Cam_MF")->GetQuaternion(&m_OriginalRot_info_2);
        // ImGui::Text("Cam_MF Orientation\n%s", VxQuaternionToString(m_OriginalRot_info_2).c_str());
        // VxQuaternion m_OriginalRot_info_3;
        // m_BML->GetTargetCameraByName("InGameCam")->GetQuaternion(&m_OriginalRot_info_3);
        // ImGui::Text("OriginalCam Orientation\n%s", VxQuaternionToString(m_OriginalRot_info_3).c_str());
        // VxQuaternion m_OriginalRot_info_4;
        // m_BML->Get3dEntityByName("Cam_OrientRef")->GetQuaternion(&m_OriginalRot_info_4);
        // ImGui::Text("OrientRef Orientation\n%s", VxQuaternionToString(m_OriginalRot_info_4).c_str());

    }
    ImGui::End();
}
