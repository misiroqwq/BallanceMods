#pragma once
#include <cmath>
#include <iostream>
#include <array>
#include <random>
#include <functional>
#include <string>
#include <regex>

#ifdef USING_BML_PLUS
# include <BML/BMLAll.h>
# ifndef m_bml
#  define m_bml m_BML
#  define m_sprite m_Sprite
#  define VT21_REF(x) &(x)
# endif
typedef const char* ICKSTRING;
#else
# include <BML/BMLAll.h>
# define VT21_REF(x) (x)
typedef CKSTRING ICKSTRING;
#endif
/// <summary>
/// draft
/// config:     |
///				v
/// enable -> get_next_position_and_prop_type() -> prop_generate(position, prop_type)
///				-> position_of_player:0
///				-> 根据视角方向、速度方向，球速度大小确定生成区
/// 
/// + 12关的光照和雷声
/// + 技能是按住可穿透至多1秒 +? 动画效果
/// </summary>
/// 文字显示 RainMode Enable
/// 通过时间片段计算速度，再利用这个速度计算区域，并生成
/// 
/// 
/// 炸开效果 -> 避免炸开而是造成碰撞
/// 
/// config内容：用相对值1-10 用户所填最大值限制
/// 间隔时间
/// 每波物件数
/// 雨范围
/// 生成位置与球速相关性
/// 物件量比例
/// 
/// bugs:


extern "C" {
	__declspec(dllexport) IMod* BMLEntry(IBML* bml);
}

class RainMode : public IMod {
public:
	RainMode(IBML* bml) : IMod(bml) {}

	virtual ICKSTRING GetID() override { return "RainMode"; }
	virtual ICKSTRING GetVersion() override { return "0.1.0 (BMLP 0.3.10)"; }
	virtual ICKSTRING GetName() override { return "RainMode"; }
	virtual ICKSTRING GetAuthor() override { return "fluoresce"; }
	virtual ICKSTRING GetDescription() override { return "RainMode"; }
	DECLARE_BML_VERSION;
	void OnLoadObject(const char* filename, CKBOOL isMap, const char* masterName, CK_CLASSID filterClass,
		CKBOOL addToScene, CKBOOL reuseMeshes, CKBOOL reuseMaterials, CKBOOL dynamic,
		XObjectArray* objArray, CKObject* masterObj) override;
	void OnLoad() override;
	void OnProcess() override;
	void OnPostResetLevel() override;
	void OnLoadScript(const char* filename, CKBehavior* script) override;
	void OnModifyConfig(ICKSTRING category, ICKSTRING key, IProperty* prop) override {load_config();}
	void OnStartLevel() override { 
		// GetLogger()->Info("test OnStartLevel");
		// m_CurSector = m_BML->GetCurrentLevel();
		next_pos_update_time = m_BML->GetTimeManager()->GetTime();
		last_camera_info = {
			.camera_vector_x = 1.00f,
			.camera_vector_z = 0,
			.camera_position_x = 0,
			.camera_position_y = 0,
			.camera_position_z = 0,
			.ball_speed_vector_x = 0,
			.ball_speed_vector_y = 0,
			.ball_speed_vector_z = 0,
			.time_stamp =0
		};
	}

private:
	// OnLoad
	bool mod_enabled = false;
	float generate_time_interval = 0;
	int entities_per_wave = 0;
	int rain_region = 0;
	float raindrop_generation_height = 0;
	float region_speed_correlation = 0;
	float raindrop_intensity_state = 0;
	const char* entities_proportion = "";
	
	IProperty* mod_enabled_config = nullptr;
	IProperty* generate_time_interval_config{};
	IProperty* entities_per_wave_config{};
	IProperty* rain_region_config{};
	IProperty* raindrop_generation_height_config{};
	IProperty* rain_region_ball_speed_correlation_config{};
	IProperty* raindrop_intensity_state_config{};
	IProperty* entities_proportion_config{};
	
	CK3dEntity* m_Balls[4] = {};

	void load_config() {
		mod_enabled = mod_enabled_config->GetBoolean();
		generate_time_interval = generate_time_interval_config->GetFloat();
		entities_per_wave = entities_per_wave_config->GetInteger();
		rain_region = rain_region_config->GetInteger();
		raindrop_generation_height = raindrop_generation_height_config->GetFloat();
		region_speed_correlation = rain_region_ball_speed_correlation_config->GetFloat();
		raindrop_intensity_state = raindrop_intensity_state_config->GetFloat();
		entities_proportion = entities_proportion_config->GetString();
		if (!isValidProportion(entities_proportion)) { entities_proportion_config->SetString("1:1:1:1"); }
	}
	
	// OnLoadObject
	CK3dEntity* m_CamOrientRef = nullptr;
	CK3dEntity* m_CamTarget = nullptr;
	
	// OnLoadScript
	void OnEditScript_Gameplay_Ingame(CKBehavior* script);
	void OnEditScript_Gameplay_Events(CKBehavior* script);
	
	// OnEditScript_Gameplay_Ingame
	CKParameterLocal* m_BallForce[2] = {};
	CKBehavior* m_DynamicPos = nullptr;
	CKBehavior* m_PhysicsNewBall = nullptr;
	CKBehavior* m_SetNewBall = nullptr;
	CKParameter* m_CurTrafo = nullptr;
	CKDataArray* m_CurLevel = nullptr;
	CKDataArray* m_IngameParam = nullptr;
	
	// OnEditScript_Gameplay_Events
	CKParameter* m_CurSector = nullptr;
	
	// OnPostResetLevel
	std::vector<std::pair<int, CK3dEntity*>> m_TempBalls;
	
	// Get_Camera_Info
	struct Camera_Info_Struct {
		float camera_vector_x;
		float camera_vector_z;
		float camera_position_x;
		float camera_position_y;
		float camera_position_z;
		float ball_speed_vector_x;
		float ball_speed_vector_y;
		float ball_speed_vector_z;
		float time_stamp;
	};
	Camera_Info_Struct last_camera_info = {};
	VxMatrix camera_matrix = {};
	
	// Random_Positions_Generator
	float ENTITY_FALL_AVG_TIME = 1.7f;
	float CAMERA_VECTOR_OFFSET = 0.3f;
	
	// Raindrop_Generator
	CK3dEntity* m_CurObj = nullptr;
	InputHook* m_InputHook = nullptr;
	
	// OnProcess
	float next_pos_update_time = 0;
	
	// .h
	bool isValidProportion(const char* entities_proportion) {
		// 判断用户所填参数是否合法
		if (entities_proportion == nullptr) { return false;}
		std::string pattern_str = R"(^\d+:\d+:\d+:\d+$)";
		std::regex pattern(pattern_str);
		return std::regex_match(entities_proportion, pattern);
	}
	
	// .cpp
	Camera_Info_Struct Get_Camera_Info();
	std::vector<VxVector> Random_Positions_Generator(auto& cur_camera_info);
	void Raindrop_Generator(const std::vector<VxVector>& positions);
};