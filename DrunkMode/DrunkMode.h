#pragma once
#include <sstream>
#include <iomanip> // 控制输出精度
#include <string>
#include <thread>
#include <memory>
#include <Windows.h>
#include <BML/Bui.h>
#include <cmath> // 用于正弦波函数 sin() 和 cos()
#include <algorithm>
#include <random>

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



extern "C" {
	__declspec(dllexport) IMod* BMLEntry(IBML* bml);
}

class DrunkMode : public IMod {
public:
	DrunkMode(IBML* bml) : IMod(bml) {}

	virtual ICKSTRING GetID() override { return "DrunkMode"; }
	virtual ICKSTRING GetVersion() override { return "0.0.1 (BMLP 0.3.10)"; }
	virtual ICKSTRING GetName() override { return "DrunkMode"; }
	virtual ICKSTRING GetAuthor() override { return "fluoresce"; }
	virtual ICKSTRING GetDescription() override { return "Makes player feel like drunk when playing."; }
	DECLARE_BML_VERSION;
	/*
	void OnLoadObject(const char* filename, CKBOOL isMap, const char* masterName, CK_CLASSID filterClass,
		CKBOOL addToScene, CKBOOL reuseMeshes, CKBOOL reuseMaterials, CKBOOL dynamic,
		XObjectArray* objArray, CKObject* masterObj) override;
	*/
	void OnLoad() override;
	void OnProcess() override;
	void OnModifyConfig(ICKSTRING category, ICKSTRING key, IProperty* prop) override { load_config(); }


private:
	// test
	// void OnDrawInfo();
	// *test

	// OnLoad
	bool mod_enabled = false;
	bool camera_enabled = false;
	bool force_enabled = false;
	float camera_intensity = 0.0f;
	float camera_frequency = 0.0f;
	float camera_displacement_scale = 0.0f;
	float force_intensity = 0.0f;
	float force_frequency = 0.0f;

	IProperty* mod_enabled_config = nullptr;
	IProperty* camera_enabled_config = nullptr;
	IProperty* force_enabled_config = nullptr;
	IProperty* camera_intensity_config = nullptr;
	IProperty* camera_frequency_config = nullptr;
	IProperty* camera_displacement_scale_config = nullptr;
	IProperty* force_intensity_config = nullptr;
	IProperty* force_frequency_config = nullptr;
	void load_config() {
		mod_enabled = mod_enabled_config->GetBoolean();
		camera_enabled = camera_enabled_config->GetBoolean();
		force_enabled = force_enabled_config->GetBoolean();
		camera_intensity = camera_intensity_config->GetFloat();
		camera_frequency = camera_frequency_config->GetFloat();
		camera_displacement_scale = camera_displacement_scale_config->GetFloat();
		force_intensity = force_intensity_config->GetFloat();
		force_frequency = force_frequency_config->GetFloat();
		if (!mod_enabled) {
			m_BML->GetRenderContext()->AttachViewpointToCamera(m_BML->GetTargetCameraByName("InGameCam"));
		}
		else {
			if (!camera_enabled) {
				m_BML->GetRenderContext()->AttachViewpointToCamera(m_BML->GetTargetCameraByName("InGameCam"));
			}
		}
	}
	CKCamera* custom_camera{};


	// ApplyDrunkEffect
	void ApplyDrunkEffect();
	float camera_TimeAccumulator = 0.0f;
	VxQuaternion CreateQuaternionFromAxisAngle(const VxVector& axis, float angle) {
		const float halfAngle = angle * 0.5f;
		const float s = sin(halfAngle);
		return VxQuaternion(
			axis.x * s,
			axis.y * s,
			axis.z * s,
			cos(halfAngle)
		);
	}
	VxQuaternion MultiplyQuaternions(const VxQuaternion& q1, const VxQuaternion& q2) {
		return VxQuaternion(
			q1.w * q2.x + q1.x * q2.w + q1.y * q2.z - q1.z * q2.y,
			q1.w * q2.y - q1.x * q2.z + q1.y * q2.w + q1.z * q2.x,
			q1.w * q2.z + q1.x * q2.y - q1.y * q2.x + q1.z * q2.w,
			q1.w * q2.w - q1.x * q2.x - q1.y * q2.y - q1.z * q2.z
		);
	}

	// ApplyNonLinearRandomXZForce
	void ApplyNonLinearRandomXZForce();
	float force_TimeAccumulator = 0.0f;

	// // utils
	// inline std::string VxQuaternionToString(const VxQuaternion& quat) {
	// 	std::ostringstream oss;
	// 	oss << std::fixed << std::setprecision(3);  // 设置精度为5位小数
	// 	oss << "("
	// 		<< quat.x << ", "
	// 		<< quat.y << ", "
	// 		<< quat.z << ", "
	// 		<< quat.w << ")";
	// 	return oss.str();
	// }
	// std::string VxVectorToString(const VxVector& vec) {
	// 	char buffer[64]; // 足够存放三个浮点数的字符串
	// 	// 格式化字符串，保留3位小数（可根据需求调整精度）
	// 	std::snprintf(
	// 		buffer,
	// 		sizeof(buffer),
	// 		"(%.3f, %.3f, %.3f)",
	// 		vec.x, vec.y, vec.z
	// 	);
	// 	return std::string(buffer); // 转为std::string返回
	// }
};