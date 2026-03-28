#pragma once
#include <cmath> // 用于正弦波函数 sin() 和 cos()
#include <algorithm>
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

#include <sstream>
#include <iomanip>
#include <string>
#include <thread>
#include <memory>
#include <Windows.h>
#include <BML/Bui.h>

extern "C" {
	__declspec(dllexport) IMod* BMLEntry(IBML* bml);
}

class ControlDirCamera : public IMod {
public:
	ControlDirCamera(IBML* bml) : IMod(bml) {}

	virtual ICKSTRING GetID() override { return "ControlDirCamera"; }
	virtual ICKSTRING GetVersion() override { return "0.0.1 (BMLP 0.3.10)"; }
	virtual ICKSTRING GetName() override { return "ControlDirCamera"; }
	virtual ICKSTRING GetAuthor() override { return "fluoresce"; }
	virtual ICKSTRING GetDescription() override { return "This is a mod designed for TAS to observe the ball's position."; }
	// 这是个用于TAS观察球位的mod，视角随实际操作方向变化，可自定义视角增量。可以用于解决转视角时计算球实际操作方向的烦琐
    BMLVersion GetBMLVersion() override { return { 0, 3, 4 }; }
	//DECLARE_BML_VERSION;
	void OnLoad() override;
	void OnProcess() override;
	void OnModifyConfig(ICKSTRING category, ICKSTRING key, IProperty* prop) override { load_config(); }
private:
	// Default Data
	/// 由quaternion_Cam_OrientRef 转化为 自定义相机可用的 quaternion 的基础转化向量
	VxQuaternion DEFAULT_QUATERNION_CUSTOM_CAMERA_OFFSET; // default -0.4836f, 0.0f, 0.0f, 0.8752f
	/// 原相机基础Fov
	float DEFAULT_FOV = 1.012291f;
	/// 由球位置 转化为 自定义相机可用的 position 的基础转化向量
	VxVector DEFAULT_POSITION_CUSTOM_CAMERA_OFFSET; // default 0.0f, 35.0f, -22.0f

	// OnLoad
	bool mod_enabled = false;
	bool showdata_enabled = false;
	float camera_fov = 0.0f;
	float camera_angle_right = 0.0f;
	float camera_angle_up = 0.0f;
	float camera_height = 0.0f;
	IProperty* mod_enabled_config = nullptr;
	IProperty* showdata_enabled_config = nullptr;
	IProperty* camera_fov_config = nullptr;
	IProperty* camera_angle_right_config = nullptr;
	IProperty* camera_angle_up_config = nullptr;
	IProperty* camera_height_config = nullptr;


	CKCamera* custom_camera{};

	void load_config() {///
		mod_enabled = mod_enabled_config->GetBoolean();
		showdata_enabled = showdata_enabled_config->GetBoolean();
		camera_fov = camera_fov_config->GetFloat();
		camera_angle_right = camera_angle_right_config->GetFloat();
		camera_angle_up = camera_angle_up_config->GetFloat();
		camera_height = camera_height_config->GetFloat();

		if (!mod_enabled) { 
			m_BML->GetRenderContext()->AttachViewpointToCamera(m_BML->GetTargetCameraByName("InGameCam")); 
		}
		if (camera_angle_up < 0.0f || camera_angle_up > 1.0f){
			camera_angle_up_config->SetFloat(0.766f);
		}

		DEFAULT_POSITION_CUSTOM_CAMERA_OFFSET = VxVector(0.0f, camera_height, -22.0f);
		DEFAULT_QUATERNION_CUSTOM_CAMERA_OFFSET = VxQuaternion(-std::sqrt(1.0f - camera_angle_up), 0.0f, 0.0f, std::sqrt(camera_angle_up));
	}


	// OnProcess
	VxQuaternion quaternion_Cam_OrientRef;
	VxVector position_Cam_OrientRef;
	VxVector position_custom_camera;
	VxQuaternion quaternion_custom_camera;
	VxQuaternion CalculateCameraQuaternion(VxQuaternion& quaternion_Cam_OrientRef, float camera_angle);
	VxVector CalculateCameraPosition(const VxVector& ballPosition, const VxQuaternion& viewQuaternion, float angle);
	void OnDrawInfo();

	// utils
	inline std::string VxQuaternionToString(const VxQuaternion& quat) {
		std::ostringstream oss;
		oss << std::fixed << std::setprecision(3);  // 设置精度为5位小数
		oss << "("
			<< quat.x << ", "
			<< quat.y << ", "
			<< quat.z << ", "
			<< quat.w << ")";
		return oss.str();
	}
	std::string VxVectorToString(const VxVector& vec) {
		char buffer[64]; // 足够存放三个浮点数的字符串
		// 格式化字符串，保留3位小数（可根据需求调整精度）
		std::snprintf(
			buffer,
			sizeof(buffer),
			"(%.3f, %.3f, %.3f)",
			vec.x, vec.y, vec.z
		);
		return std::string(buffer); // 转为std::string返回
	}
};