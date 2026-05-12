#pragma once
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
#include <cmath>
#include <algorithm>
#include <regex>
#include <set>

extern "C" {
	__declspec(dllexport) IMod* BMLEntry(IBML* bml);
}

class ExtraKeyFunctionForTAS : public IMod {
public:
	ExtraKeyFunctionForTAS(IBML* bml) : IMod(bml) {}

	virtual ICKSTRING GetID() override { return "ExtraKeyFunctionForTAS"; }
	virtual ICKSTRING GetVersion() override { return "0.0.1 (BMLP 0.3.4)"; }
	virtual ICKSTRING GetName() override { return "ExtraKeyFunctionForTAS"; }
	virtual ICKSTRING GetAuthor() override { return "fluoresce"; }
	virtual ICKSTRING GetDescription() override { return "Adding extra functions to keys for TAS"; }
	BMLVersion GetBMLVersion() override { return { 0, 3, 4 }; }
	//DECLARE_BML_VERSION;
	void OnLoad() override;
	void OnProcess() override;
	void OnModifyConfig(ICKSTRING category, ICKSTRING key, IProperty* prop) override { load_config(); }
	void OnBallNavActive() override { Ball_Active = true; };
	void OnBallNavInactive() override { Ball_Active = false; };
	void OnPostStartMenu() override;
	void OnPostResetLevel() override;
	void OnLoadScript(const char* filename, CKBehavior* script) override;
private:
	// OnLoad
	bool mod_enabled = false;
	const char* TP_info = "";
	const char* Summon_info = "";

	IProperty* mod_enabled_config = nullptr;
	IProperty* TP_info_config{};
	IProperty* Summon_info_config{};

	CK3dEntity* m_Balls[4] = {};

	void load_config() {
		mod_enabled = mod_enabled_config->GetBoolean();
		TP_info = TP_info_config->GetString();
		Summon_info = Summon_info_config->GetString();
		if (!mod_enabled) { return; }
		if (!ParseTPInfoString(TP_info)) {
			TP_info_config->SetString("");
			m_BML->SendIngameMessage(("ExtraKeyFunctionForTAS: TP_info Parameter Error"));
		}
		if (!ParseSummonInfoString(Summon_info)) {
			Summon_info_config->SetString("");
			m_BML->SendIngameMessage(("ExtraKeyFunctionForTAS: Summon_info Parameter Error"));
		}
	};

	// OnLoadScript
	CKBehavior* m_dynamicPos = nullptr;
	CKBehavior* m_phyNewBall = nullptr;


	// OnPostStartMenu
	bool init = false;
	CKDataArray* m_checkpoints = nullptr;
	CKDataArray* m_resetpoints = nullptr;
	CKDataArray* m_ingameParam = nullptr;
	CKParameter* m_curSector = nullptr;
	CKParameter* m_curTrafo = nullptr;
	CKBehavior* m_setNewBall = nullptr;
	CKDataArray* m_curLevel = nullptr;

	// OnPostResetLevel
	std::vector<std::pair<int, CK3dEntity *>> m_TempBalls;

	// OnEditScript_Gameplay_Events
	void OnEditScript_Gameplay_Events(CKBehavior* script);
	CKParameter* m_CurSector = nullptr;

	// OnProcess
	void OnDrawInfo();
	void BallUp();
	void TPWithSpace();
	void SummonWithEnter();
	bool Ball_Active = false;
	VxMatrix matrix;
	/// 
	int CurrentIndex_TP = -1;
	struct TP_Info_Data {
		int TP_sector;
		std::string TP_balltype;
		float TP_position[3]; // 包含 3 个数字
	};
	std::vector<TP_Info_Data> Parse_TP_Info;
	///
	int CurrentIndex_Summon = -1;
	struct Summon_Info_Data {
		std::string Summon_balltype;
		float Summon_position[3];
	};
	std::vector<Summon_Info_Data> Parse_Summon_Info;
	///


	// TPwithSpace
	static void SetParamString(CKParameter* param, CKSTRING value) {
		param->SetStringValue(value);
	}
	// .h
	bool ParseSummonInfoString(const char* input) {
		if (input == nullptr || input[0] == '\0') {
			return true;
		}
		std::string str(input);
		// 1. 去除所有空格，方便正则处理
		str.erase(std::remove(str.begin(), str.end(), ' '), str.end());

		if (str.empty()) return false;

		// 2. 定义正则表达式
		// 解释：
		// \( : 匹配左括号
		// (\d+) : 捕获组1，匹配第一个整数
		// ,(paper|wood|stone), : 捕获组2，匹配指定的字符串类型
		// ([-\d.]+),([-\d.]+),([-\d.]+) : 捕获组3,4,5，匹配可能是小数的数字
		// \) : 匹配右括号
		std::regex pattern(R"(\((paper|wood|stone|box),([-\d.]+),([-\d.]+),([-\d.]+)\))");

		auto words_begin = std::sregex_iterator(str.begin(), str.end(), pattern);
		auto words_end = std::sregex_iterator();

		size_t last_pos = 0;
		Parse_Summon_Info.clear();

		for (std::sregex_iterator i = words_begin; i != words_end; ++i) {
			std::smatch match = *i;

			// 验证每一组数据之间的连接符（必须是逗号，或者是第一组）
			if (i->position() > 0) {
				// 检查上一组结束到这一组开始之间是否有非法字符
				std::string gap = str.substr(last_pos, i->position() - last_pos);
				if (gap != ",") return false;
			}

			// 解析数据

			Summon_Info_Data data;
			data.Summon_balltype = match[1].str();
			data.Summon_position[0] = std::stof(match[2].str());
			data.Summon_position[1] = std::stof(match[3].str());
			data.Summon_position[2] = std::stof(match[4].str());

			Parse_Summon_Info.push_back(data);
			last_pos = i->position() + i->length();
		}

		// 检查是否处理到了字符串末尾，防止后面跟着乱码
		if (last_pos != str.length()) return false;

		// 如果一个匹配项都没有找到
		return !Parse_Summon_Info.empty();
	};
	bool ParseTPInfoString(const char* input) {
		if (input == nullptr || input[0] == '\0') {
			return true;
		}
		std::string str(input);
		// 1. 去除所有空格，方便正则处理
		str.erase(std::remove(str.begin(), str.end(), ' '), str.end());

		if (str.empty()) return false;

		// 2. 定义正则表达式
		// 解释：
		// \( : 匹配左括号
		// (\d+) : 捕获组1，匹配第一个整数
		// ,(paper|wood|stone), : 捕获组2，匹配指定的字符串类型
		// ([-\d.]+),([-\d.]+),([-\d.]+) : 捕获组3,4,5，匹配可能是小数的数字
		// \) : 匹配右括号
		std::regex pattern(R"(\((\d+),(paper|wood|stone),([-\d.]+),([-\d.]+),([-\d.]+)\))");

		auto words_begin = std::sregex_iterator(str.begin(), str.end(), pattern);
		auto words_end = std::sregex_iterator();

		size_t last_pos = 0;
		Parse_TP_Info.clear();

		for (std::sregex_iterator i = words_begin; i != words_end; ++i) {
			std::smatch match = *i;

			// 验证每一组数据之间的连接符（必须是逗号，或者是第一组）
			if (i->position() > 0) {
				// 检查上一组结束到这一组开始之间是否有非法字符
				std::string gap = str.substr(last_pos, i->position() - last_pos);
				if (gap != ",") return false;
			}

			// 解析数据
			int sector = std::stoi(match[1].str());
			if (sector <= 0) return false; // 校验：第一个值必须 > 0

			TP_Info_Data data;
			data.TP_sector = sector;
			data.TP_balltype = match[2].str();
			data.TP_position[0] = std::stof(match[3].str());
			data.TP_position[1] = std::stof(match[4].str());
			data.TP_position[2] = std::stof(match[5].str());

			Parse_TP_Info.push_back(data);
			last_pos = i->position() + i->length();
		}

		// 检查是否处理到了字符串末尾，防止后面跟着乱码
		if (last_pos != str.length()) return false;

		// 如果一个匹配项都没有找到
		return !Parse_TP_Info.empty();
	};
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
	// };
};