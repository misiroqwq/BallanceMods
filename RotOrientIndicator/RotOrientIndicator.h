#pragma once
#include <cmath>
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

class RotOrientIndicator : public IMod {


public:
    RotOrientIndicator(IBML* bml) : IMod(bml) {}

    virtual ICKSTRING GetID() override { return "RotOrientIndicator"; }
    virtual ICKSTRING GetVersion() override { return "0.0.1"; }
    virtual ICKSTRING GetName() override { return "RotOrientIndicator"; }
    virtual ICKSTRING GetAuthor() override { return "fluoresce"; }
    virtual ICKSTRING GetDescription() override { return "To more clearly display the direction and speed of the ball's rotation"; }
    BMLVersion GetBMLVersion() override { return { 0, 3, 4 }; }
    // DECLARE_BML_VERSION;


    void OnLoad() override;
    void OnProcess() override;
    void OnModifyConfig(ICKSTRING category, ICKSTRING key, IProperty* prop) override { load_config(); }
    void OnPostStartMenu() override;


private:
    // OnProcess
    float axisX, axisY, axisZ;
    float RotationVelocity;
    void CalculateRotationAxis(const VxQuaternion& prevQuat, const VxQuaternion& currQuat, float& axisX, float& axisY, float& axisZ);
    VxQuaternion CreateQuaternionFromVector(float axisX, float axisY, float axisZ);
    float LinearMap(float input);
    float CalculateRotationVelocity(const VxQuaternion& prevQuat, const VxQuaternion& currQuat);
    void OnDrawInfo();
    //OnPostStartMenu

    CKDataArray* current_level_array{};
    CK3dObject* indicator{};
    VxQuaternion PreBallVxQuaternion;
    CK3dEntity* current_ball = nullptr;
    CKParameter* m_ActiveBall = nullptr;
    bool init = false;
    // OnLoad
    bool mod_enabled = false;
    bool show_info_enabled = false;
    bool rotation_effect_enabled = false;
    bool scaling_effect_enabled = false;
    float scaling_effect_max_scale = 0.0f;
    float scaling_effect_max_velocity = 0.0f;
    IProperty* mod_enabled_config = nullptr;
    IProperty* show_info_enabled_config = nullptr;
    IProperty* rotation_effect_enabled_config = nullptr;
    IProperty* scaling_effect_enabled_config = nullptr;
    IProperty* scaling_effect_max_scale_config = nullptr;
    IProperty* scaling_effect_max_velocity_config = nullptr;
    void load_config() { 
        mod_enabled = mod_enabled_config->GetBoolean(); 
        show_info_enabled = show_info_enabled_config->GetBoolean();
        rotation_effect_enabled = rotation_effect_enabled_config->GetBoolean();
        scaling_effect_enabled = scaling_effect_enabled_config->GetBoolean();
        scaling_effect_max_scale = scaling_effect_max_scale_config->GetFloat();
        scaling_effect_max_velocity = scaling_effect_max_velocity_config->GetFloat();
    }
};
