#pragma once

#if defined(ENABLE_PLAYFABADMIN_API)

#include <playfab/PlayFabBaseModel.h>
#include <playfab/PlayFabJsonHeaders.h>

namespace PlayFab
{
    namespace AdminModels
    {
        // Admin Enums
        enum class AuthTokenType
        {
            AuthTokenTypeEmail
        };

        inline void ToJsonEnum(const AuthTokenType input, Json::Value& output)
        {
            if (input == AuthTokenType::AuthTokenTypeEmail)
            {
                output = Json::Value("Email");
                return;
            }
        }
        inline void FromJsonEnum(const Json::Value& input, AuthTokenType& output)
        {
            if (!input.isString())
            {
                return;
            }
            const std::string& inputStr = input.asString();
            if (inputStr == "Email")
            {
                output = AuthTokenType::AuthTokenTypeEmail;
                return;
            }
        }

        enum class Conditionals
        {
            ConditionalsAny,
            ConditionalsTrue,
            ConditionalsFalse
        };

        inline void ToJsonEnum(const Conditionals input, Json::Value& output)
        {
            if (input == Conditionals::ConditionalsAny)
            {
                output = Json::Value("Any");
                return;
            }
            if (input == Conditionals::ConditionalsTrue)
            {
                output = Json::Value("True");
                return;
            }
            if (input == Conditionals::ConditionalsFalse)
            {
                output = Json::Value("False");
                return;
            }
        }
        inline void FromJsonEnum(const Json::Value& input, Conditionals& output)
        {
            if (!input.isString())
            {
                return;
            }
            const std::string& inputStr = input.asString();
            if (inputStr == "Any")
            {
                output = Conditionals::ConditionalsAny;
                return;
            }
            if (inputStr == "True")
            {
                output = Conditionals::ConditionalsTrue;
                return;
            }
            if (inputStr == "False")
            {
                output = Conditionals::ConditionalsFalse;
                return;
            }
        }

        enum class ContinentCode
        {
            ContinentCodeAF,
            ContinentCodeAN,
            ContinentCodeAS,
            ContinentCodeEU,
            ContinentCodeNA,
            ContinentCodeOC,
            ContinentCodeSA
        };

        inline void ToJsonEnum(const ContinentCode input, Json::Value& output)
        {
            if (input == ContinentCode::ContinentCodeAF)
            {
                output = Json::Value("AF");
                return;
            }
            if (input == ContinentCode::ContinentCodeAN)
            {
                output = Json::Value("AN");
                return;
            }
            if (input == ContinentCode::ContinentCodeAS)
            {
                output = Json::Value("AS");
                return;
            }
            if (input == ContinentCode::ContinentCodeEU)
            {
                output = Json::Value("EU");
                return;
            }
            if (input == ContinentCode::ContinentCodeNA)
            {
                output = Json::Value("NA");
                return;
            }
            if (input == ContinentCode::ContinentCodeOC)
            {
                output = Json::Value("OC");
                return;
            }
            if (input == ContinentCode::ContinentCodeSA)
            {
                output = Json::Value("SA");
                return;
            }
        }
        inline void FromJsonEnum(const Json::Value& input, ContinentCode& output)
        {
            if (!input.isString())
            {
                return;
            }
            const std::string& inputStr = input.asString();
            if (inputStr == "AF")
            {
                output = ContinentCode::ContinentCodeAF;
                return;
            }
            if (inputStr == "AN")
            {
                output = ContinentCode::ContinentCodeAN;
                return;
            }
            if (inputStr == "AS")
            {
                output = ContinentCode::ContinentCodeAS;
                return;
            }
            if (inputStr == "EU")
            {
                output = ContinentCode::ContinentCodeEU;
                return;
            }
            if (inputStr == "NA")
            {
                output = ContinentCode::ContinentCodeNA;
                return;
            }
            if (inputStr == "OC")
            {
                output = ContinentCode::ContinentCodeOC;
                return;
            }
            if (inputStr == "SA")
            {
                output = ContinentCode::ContinentCodeSA;
                return;
            }
        }

        enum class CountryCode
        {
            CountryCodeAF,
            CountryCodeAX,
            CountryCodeAL,
            CountryCodeDZ,
            CountryCodeAS,
            CountryCodeAD,
            CountryCodeAO,
            CountryCodeAI,
            CountryCodeAQ,
            CountryCodeAG,
            CountryCodeAR,
            CountryCodeAM,
            CountryCodeAW,
            CountryCodeAU,
            CountryCodeAT,
            CountryCodeAZ,
            CountryCodeBS,
            CountryCodeBH,
            CountryCodeBD,
            CountryCodeBB,
            CountryCodeBY,
            CountryCodeBE,
            CountryCodeBZ,
            CountryCodeBJ,
            CountryCodeBM,
            CountryCodeBT,
            CountryCodeBO,
            CountryCodeBQ,
            CountryCodeBA,
            CountryCodeBW,
            CountryCodeBV,
            CountryCodeBR,
            CountryCodeIO,
            CountryCodeBN,
            CountryCodeBG,
            CountryCodeBF,
            CountryCodeBI,
            CountryCodeKH,
            CountryCodeCM,
            CountryCodeCA,
            CountryCodeCV,
            CountryCodeKY,
            CountryCodeCF,
            CountryCodeTD,
            CountryCodeCL,
            CountryCodeCN,
            CountryCodeCX,
            CountryCodeCC,
            CountryCodeCO,
            CountryCodeKM,
            CountryCodeCG,
            CountryCodeCD,
            CountryCodeCK,
            CountryCodeCR,
            CountryCodeCI,
            CountryCodeHR,
            CountryCodeCU,
            CountryCodeCW,
            CountryCodeCY,
            CountryCodeCZ,
            CountryCodeDK,
            CountryCodeDJ,
            CountryCodeDM,
            CountryCodeDO,
            CountryCodeEC,
            CountryCodeEG,
            CountryCodeSV,
            CountryCodeGQ,
            CountryCodeER,
            CountryCodeEE,
            CountryCodeET,
            CountryCodeFK,
            CountryCodeFO,
            CountryCodeFJ,
            CountryCodeFI,
            CountryCodeFR,
            CountryCodeGF,
            CountryCodePF,
            CountryCodeTF,
            CountryCodeGA,
            CountryCodeGM,
            CountryCodeGE,
            CountryCodeDE,
            CountryCodeGH,
            CountryCodeGI,
            CountryCodeGR,
            CountryCodeGL,
            CountryCodeGD,
            CountryCodeGP,
            CountryCodeGU,
            CountryCodeGT,
            CountryCodeGG,
            CountryCodeGN,
            CountryCodeGW,
            CountryCodeGY,
            CountryCodeHT,
            CountryCodeHM,
            CountryCodeVA,
            CountryCodeHN,
            CountryCodeHK,
            CountryCodeHU,
            CountryCodeIS,
            CountryCodeIN,
            CountryCodeID,
            CountryCodeIR,
            CountryCodeIQ,
            CountryCodeIE,
            CountryCodeIM,
            CountryCodeIL,
            CountryCodeIT,
            CountryCodeJM,
            CountryCodeJP,
            CountryCodeJE,
            CountryCodeJO,
            CountryCodeKZ,
            CountryCodeKE,
            CountryCodeKI,
            CountryCodeKP,
            CountryCodeKR,
            CountryCodeKW,
            CountryCodeKG,
            CountryCodeLA,
            CountryCodeLV,
            CountryCodeLB,
            CountryCodeLS,
            CountryCodeLR,
            CountryCodeLY,
            CountryCodeLI,
            CountryCodeLT,
            CountryCodeLU,
            CountryCodeMO,
            CountryCodeMK,
            CountryCodeMG,
            CountryCodeMW,
            CountryCodeMY,
            CountryCodeMV,
            CountryCodeML,
            CountryCodeMT,
            CountryCodeMH,
            CountryCodeMQ,
            CountryCodeMR,
            CountryCodeMU,
            CountryCodeYT,
            CountryCodeMX,
            CountryCodeFM,
            CountryCodeMD,
            CountryCodeMC,
            CountryCodeMN,
            CountryCodeME,
            CountryCodeMS,
            CountryCodeMA,
            CountryCodeMZ,
            CountryCodeMM,
            CountryCodeNA,
            CountryCodeNR,
            CountryCodeNP,
            CountryCodeNL,
            CountryCodeNC,
            CountryCodeNZ,
            CountryCodeNI,
            CountryCodeNE,
            CountryCodeNG,
            CountryCodeNU,
            CountryCodeNF,
            CountryCodeMP,
            CountryCodeNO,
            CountryCodeOM,
            CountryCodePK,
            CountryCodePW,
            CountryCodePS,
            CountryCodePA,
            CountryCodePG,
            CountryCodePY,
            CountryCodePE,
            CountryCodePH,
            CountryCodePN,
            CountryCodePL,
            CountryCodePT,
            CountryCodePR,
            CountryCodeQA,
            CountryCodeRE,
            CountryCodeRO,
            CountryCodeRU,
            CountryCodeRW,
            CountryCodeBL,
            CountryCodeSH,
            CountryCodeKN,
            CountryCodeLC,
            CountryCodeMF,
            CountryCodePM,
            CountryCodeVC,
            CountryCodeWS,
            CountryCodeSM,
            CountryCodeST,
            CountryCodeSA,
            CountryCodeSN,
            CountryCodeRS,
            CountryCodeSC,
            CountryCodeSL,
            CountryCodeSG,
            CountryCodeSX,
            CountryCodeSK,
            CountryCodeSI,
            CountryCodeSB,
            CountryCodeSO,
            CountryCodeZA,
            CountryCodeGS,
            CountryCodeSS,
            CountryCodeES,
            CountryCodeLK,
            CountryCodeSD,
            CountryCodeSR,
            CountryCodeSJ,
            CountryCodeSZ,
            CountryCodeSE,
            CountryCodeCH,
            CountryCodeSY,
            CountryCodeTW,
            CountryCodeTJ,
            CountryCodeTZ,
            CountryCodeTH,
            CountryCodeTL,
            CountryCodeTG,
            CountryCodeTK,
            CountryCodeTO,
            CountryCodeTT,
            CountryCodeTN,
            CountryCodeTR,
            CountryCodeTM,
            CountryCodeTC,
            CountryCodeTV,
            CountryCodeUG,
            CountryCodeUA,
            CountryCodeAE,
            CountryCodeGB,
            CountryCodeUS,
            CountryCodeUM,
            CountryCodeUY,
            CountryCodeUZ,
            CountryCodeVU,
            CountryCodeVE,
            CountryCodeVN,
            CountryCodeVG,
            CountryCodeVI,
            CountryCodeWF,
            CountryCodeEH,
            CountryCodeYE,
            CountryCodeZM,
            CountryCodeZW
        };

        inline void ToJsonEnum(const CountryCode input, Json::Value& output)
        {
            if (input == CountryCode::CountryCodeAF)
            {
                output = Json::Value("AF");
                return;
            }
            if (input == CountryCode::CountryCodeAX)
            {
                output = Json::Value("AX");
                return;
            }
            if (input == CountryCode::CountryCodeAL)
            {
                output = Json::Value("AL");
                return;
            }
            if (input == CountryCode::CountryCodeDZ)
            {
                output = Json::Value("DZ");
                return;
            }
            if (input == CountryCode::CountryCodeAS)
            {
                output = Json::Value("AS");
                return;
            }
            if (input == CountryCode::CountryCodeAD)
            {
                output = Json::Value("AD");
                return;
            }
            if (input == CountryCode::CountryCodeAO)
            {
                output = Json::Value("AO");
                return;
            }
            if (input == CountryCode::CountryCodeAI)
            {
                output = Json::Value("AI");
                return;
            }
            if (input == CountryCode::CountryCodeAQ)
            {
                output = Json::Value("AQ");
                return;
            }
            if (input == CountryCode::CountryCodeAG)
            {
                output = Json::Value("AG");
                return;
            }
            if (input == CountryCode::CountryCodeAR)
            {
                output = Json::Value("AR");
                return;
            }
            if (input == CountryCode::CountryCodeAM)
            {
                output = Json::Value("AM");
                return;
            }
            if (input == CountryCode::CountryCodeAW)
            {
                output = Json::Value("AW");
                return;
            }
            if (input == CountryCode::CountryCodeAU)
            {
                output = Json::Value("AU");
                return;
            }
            if (input == CountryCode::CountryCodeAT)
            {
                output = Json::Value("AT");
                return;
            }
            if (input == CountryCode::CountryCodeAZ)
            {
                output = Json::Value("AZ");
                return;
            }
            if (input == CountryCode::CountryCodeBS)
            {
                output = Json::Value("BS");
                return;
            }
            if (input == CountryCode::CountryCodeBH)
            {
                output = Json::Value("BH");
                return;
            }
            if (input == CountryCode::CountryCodeBD)
            {
                output = Json::Value("BD");
                return;
            }
            if (input == CountryCode::CountryCodeBB)
            {
                output = Json::Value("BB");
                return;
            }
            if (input == CountryCode::CountryCodeBY)
            {
                output = Json::Value("BY");
                return;
            }
            if (input == CountryCode::CountryCodeBE)
            {
                output = Json::Value("BE");
                return;
            }
            if (input == CountryCode::CountryCodeBZ)
            {
                output = Json::Value("BZ");
                return;
            }
            if (input == CountryCode::CountryCodeBJ)
            {
                output = Json::Value("BJ");
                return;
            }
            if (input == CountryCode::CountryCodeBM)
            {
                output = Json::Value("BM");
                return;
            }
            if (input == CountryCode::CountryCodeBT)
            {
                output = Json::Value("BT");
                return;
            }
            if (input == CountryCode::CountryCodeBO)
            {
                output = Json::Value("BO");
                return;
            }
            if (input == CountryCode::CountryCodeBQ)
            {
                output = Json::Value("BQ");
                return;
            }
            if (input == CountryCode::CountryCodeBA)
            {
                output = Json::Value("BA");
                return;
            }
            if (input == CountryCode::CountryCodeBW)
            {
                output = Json::Value("BW");
                return;
            }
            if (input == CountryCode::CountryCodeBV)
            {
                output = Json::Value("BV");
                return;
            }
            if (input == CountryCode::CountryCodeBR)
            {
                output = Json::Value("BR");
                return;
            }
            if (input == CountryCode::CountryCodeIO)
            {
                output = Json::Value("IO");
                return;
            }
            if (input == CountryCode::CountryCodeBN)
            {
                output = Json::Value("BN");
                return;
            }
            if (input == CountryCode::CountryCodeBG)
            {
                output = Json::Value("BG");
                return;
            }
            if (input == CountryCode::CountryCodeBF)
            {
                output = Json::Value("BF");
                return;
            }
            if (input == CountryCode::CountryCodeBI)
            {
                output = Json::Value("BI");
                return;
            }
            if (input == CountryCode::CountryCodeKH)
            {
                output = Json::Value("KH");
                return;
            }
            if (input == CountryCode::CountryCodeCM)
            {
                output = Json::Value("CM");
                return;
            }
            if (input == CountryCode::CountryCodeCA)
            {
                output = Json::Value("CA");
                return;
            }
            if (input == CountryCode::CountryCodeCV)
            {
                output = Json::Value("CV");
                return;
            }
            if (input == CountryCode::CountryCodeKY)
            {
                output = Json::Value("KY");
                return;
            }
            if (input == CountryCode::CountryCodeCF)
            {
                output = Json::Value("CF");
                return;
            }
            if (input == CountryCode::CountryCodeTD)
            {
                output = Json::Value("TD");
                return;
            }
            if (input == CountryCode::CountryCodeCL)
            {
                output = Json::Value("CL");
                return;
            }
            if (input == CountryCode::CountryCodeCN)
            {
                output = Json::Value("CN");
                return;
            }
            if (input == CountryCode::CountryCodeCX)
            {
                output = Json::Value("CX");
                return;
            }
            if (input == CountryCode::CountryCodeCC)
            {
                output = Json::Value("CC");
                return;
            }
            if (input == CountryCode::CountryCodeCO)
            {
                output = Json::Value("CO");
                return;
            }
            if (input == CountryCode::CountryCodeKM)
            {
                output = Json::Value("KM");
                return;
            }
            if (input == CountryCode::CountryCodeCG)
            {
                output = Json::Value("CG");
                return;
            }
            if (input == CountryCode::CountryCodeCD)
            {
                output = Json::Value("CD");
                return;
            }
            if (input == CountryCode::CountryCodeCK)
            {
                output = Json::Value("CK");
                return;
            }
            if (input == CountryCode::CountryCodeCR)
            {
                output = Json::Value("CR");
                return;
            }
            if (input == CountryCode::CountryCodeCI)
            {
                output = Json::Value("CI");
                return;
            }
            if (input == CountryCode::CountryCodeHR)
            {
                output = Json::Value("HR");
                return;
            }
            if (input == CountryCode::CountryCodeCU)
            {
                output = Json::Value("CU");
                return;
            }
            if (input == CountryCode::CountryCodeCW)
            {
                output = Json::Value("CW");
                return;
            }
            if (input == CountryCode::CountryCodeCY)
            {
                output = Json::Value("CY");
                return;
            }
            if (input == CountryCode::CountryCodeCZ)
            {
                output = Json::Value("CZ");
                return;
            }
            if (input == CountryCode::CountryCodeDK)
            {
                output = Json::Value("DK");
                return;
            }
            if (input == CountryCode::CountryCodeDJ)
            {
                output = Json::Value("DJ");
                return;
            }
            if (input == CountryCode::CountryCodeDM)
            {
                output = Json::Value("DM");
                return;
            }
            if (input == CountryCode::CountryCodeDO)
            {
                output = Json::Value("DO");
                return;
            }
            if (input == CountryCode::CountryCodeEC)
            {
                output = Json::Value("EC");
                return;
            }
            if (input == CountryCode::CountryCodeEG)
            {
                output = Json::Value("EG");
                return;
            }
            if (input == CountryCode::CountryCodeSV)
            {
                output = Json::Value("SV");
                return;
            }
            if (input == CountryCode::CountryCodeGQ)
            {
                output = Json::Value("GQ");
                return;
            }
            if (input == CountryCode::CountryCodeER)
            {
                output = Json::Value("ER");
                return;
            }
            if (input == CountryCode::CountryCodeEE)
            {
                output = Json::Value("EE");
                return;
            }
            if (input == CountryCode::CountryCodeET)
            {
                output = Json::Value("ET");
                return;
            }
            if (input == CountryCode::CountryCodeFK)
            {
                output = Json::Value("FK");
                return;
            }
            if (input == CountryCode::CountryCodeFO)
            {
                output = Json::Value("FO");
                return;
            }
            if (input == CountryCode::CountryCodeFJ)
            {
                output = Json::Value("FJ");
                return;
            }
            if (input == CountryCode::CountryCodeFI)
            {
                output = Json::Value("FI");
                return;
            }
            if (input == CountryCode::CountryCodeFR)
            {
                output = Json::Value("FR");
                return;
            }
            if (input == CountryCode::CountryCodeGF)
            {
                output = Json::Value("GF");
                return;
            }
            if (input == CountryCode::CountryCodePF)
            {
                output = Json::Value("PF");
                return;
            }
            if (input == CountryCode::CountryCodeTF)
            {
                output = Json::Value("TF");
                return;
            }
            if (input == CountryCode::CountryCodeGA)
            {
                output = Json::Value("GA");
                return;
            }
            if (input == CountryCode::CountryCodeGM)
            {
                output = Json::Value("GM");
                return;
            }
            if (input == CountryCode::CountryCodeGE)
            {
                output = Json::Value("GE");
                return;
            }
            if (input == CountryCode::CountryCodeDE)
            {
                output = Json::Value("DE");
                return;
            }
            if (input == CountryCode::CountryCodeGH)
            {
                output = Json::Value("GH");
                return;
            }
            if (input == CountryCode::CountryCodeGI)
            {
                output = Json::Value("GI");
                return;
            }
            if (input == CountryCode::CountryCodeGR)
            {
                output = Json::Value("GR");
                return;
            }
            if (input == CountryCode::CountryCodeGL)
            {
                output = Json::Value("GL");
                return;
            }
            if (input == CountryCode::CountryCodeGD)
            {
                output = Json::Value("GD");
                return;
            }
            if (input == CountryCode::CountryCodeGP)
            {
                output = Json::Value("GP");
                return;
            }
            if (input == CountryCode::CountryCodeGU)
            {
                output = Json::Value("GU");
                return;
            }
            if (input == CountryCode::CountryCodeGT)
            {
                output = Json::Value("GT");
                return;
            }
            if (input == CountryCode::CountryCodeGG)
            {
                output = Json::Value("GG");
                return;
            }
            if (input == CountryCode::CountryCodeGN)
            {
                output = Json::Value("GN");
                return;
            }
            if (input == CountryCode::CountryCodeGW)
            {
                output = Json::Value("GW");
                return;
            }
            if (input == CountryCode::CountryCodeGY)
            {
                output = Json::Value("GY");
                return;
            }
            if (input == CountryCode::CountryCodeHT)
            {
                output = Json::Value("HT");
                return;
            }
            if (input == CountryCode::CountryCodeHM)
            {
                output = Json::Value("HM");
                return;
            }
            if (input == CountryCode::CountryCodeVA)
            {
                output = Json::Value("VA");
                return;
            }
            if (input == CountryCode::CountryCodeHN)
            {
                output = Json::Value("HN");
                return;
            }
            if (input == CountryCode::CountryCodeHK)
            {
                output = Json::Value("HK");
                return;
            }
            if (input == CountryCode::CountryCodeHU)
            {
                output = Json::Value("HU");
                return;
            }
            if (input == CountryCode::CountryCodeIS)
            {
                output = Json::Value("IS");
                return;
            }
            if (input == CountryCode::CountryCodeIN)
            {
                output = Json::Value("IN");
                return;
            }
            if (input == CountryCode::CountryCodeID)
            {
                output = Json::Value("ID");
                return;
            }
            if (input == CountryCode::CountryCodeIR)
            {
                output = Json::Value("IR");
                return;
            }
            if (input == CountryCode::CountryCodeIQ)
            {
                output = Json::Value("IQ");
                return;
            }
            if (input == CountryCode::CountryCodeIE)
            {
                output = Json::Value("IE");
                return;
            }
            if (input == CountryCode::CountryCodeIM)
            {
                output = Json::Value("IM");
                return;
            }
            if (input == CountryCode::CountryCodeIL)
            {
                output = Json::Value("IL");
                return;
            }
            if (input == CountryCode::CountryCodeIT)
            {
                output = Json::Value("IT");
                return;
            }
            if (input == CountryCode::CountryCodeJM)
            {
                output = Json::Value("JM");
                return;
            }
            if (input == CountryCode::CountryCodeJP)
            {
                output = Json::Value("JP");
                return;
            }
            if (input == CountryCode::CountryCodeJE)
            {
                output = Json::Value("JE");
                return;
            }
            if (input == CountryCode::CountryCodeJO)
            {
                output = Json::Value("JO");
                return;
            }
            if (input == CountryCode::CountryCodeKZ)
            {
                output = Json::Value("KZ");
                return;
            }
            if (input == CountryCode::CountryCodeKE)
            {
                output = Json::Value("KE");
                return;
            }
            if (input == CountryCode::CountryCodeKI)
            {
                output = Json::Value("KI");
                return;
            }
            if (input == CountryCode::CountryCodeKP)
            {
                output = Json::Value("KP");
                return;
            }
            if (input == CountryCode::CountryCodeKR)
            {
                output = Json::Value("KR");
                return;
            }
            if (input == CountryCode::CountryCodeKW)
            {
                output = Json::Value("KW");
                return;
            }
            if (input == CountryCode::CountryCodeKG)
            {
                output = Json::Value("KG");
                return;
            }
            if (input == CountryCode::CountryCodeLA)
            {
                output = Json::Value("LA");
                return;
            }
            if (input == CountryCode::CountryCodeLV)
            {
                output = Json::Value("LV");
                return;
            }
            if (input == CountryCode::CountryCodeLB)
            {
                output = Json::Value("LB");
                return;
            }
            if (input == CountryCode::CountryCodeLS)
            {
                output = Json::Value("LS");
                return;
            }
            if (input == CountryCode::CountryCodeLR)
            {
                output = Json::Value("LR");
                return;
            }
            if (input == CountryCode::CountryCodeLY)
            {
                output = Json::Value("LY");
                return;
            }
            if (input == CountryCode::CountryCodeLI)
            {
                output = Json::Value("LI");
                return;
            }
            if (input == CountryCode::CountryCodeLT)
            {
                output = Json::Value("LT");
                return;
            }
            if (input == CountryCode::CountryCodeLU)
            {
                output = Json::Value("LU");
                return;
            }
            if (input == CountryCode::CountryCodeMO)
            {
                output = Json::Value("MO");
                return;
            }
            if (input == CountryCode::CountryCodeMK)
            {
                output = Json::Value("MK");
                return;
            }
            if (input == CountryCode::CountryCodeMG)
            {
                output = Json::Value("MG");
                return;
            }
            if (input == CountryCode::CountryCodeMW)
            {
                output = Json::Value("MW");
                return;
            }
            if (input == CountryCode::CountryCodeMY)
            {
                output = Json::Value("MY");
                return;
            }
            if (input == CountryCode::CountryCodeMV)
            {
                output = Json::Value("MV");
                return;
            }
            if (input == CountryCode::CountryCodeML)
            {
                output = Json::Value("ML");
                return;
            }
            if (input == CountryCode::CountryCodeMT)
            {
                output = Json::Value("MT");
                return;
            }
            if (input == CountryCode::CountryCodeMH)
            {
                output = Json::Value("MH");
                return;
            }
            if (input == CountryCode::CountryCodeMQ)
            {
                output = Json::Value("MQ");
                return;
            }
            if (input == CountryCode::CountryCodeMR)
            {
                output = Json::Value("MR");
                return;
            }
            if (input == CountryCode::CountryCodeMU)
            {
                output = Json::Value("MU");
                return;
            }
            if (input == CountryCode::CountryCodeYT)
            {
                output = Json::Value("YT");
                return;
            }
            if (input == CountryCode::CountryCodeMX)
            {
                output = Json::Value("MX");
                return;
            }
            if (input == CountryCode::CountryCodeFM)
            {
                output = Json::Value("FM");
                return;
            }
            if (input == CountryCode::CountryCodeMD)
            {
                output = Json::Value("MD");
                return;
            }
            if (input == CountryCode::CountryCodeMC)
            {
                output = Json::Value("MC");
                return;
            }
            if (input == CountryCode::CountryCodeMN)
            {
                output = Json::Value("MN");
                return;
            }
            if (input == CountryCode::CountryCodeME)
            {
                output = Json::Value("ME");
                return;
            }
            if (input == CountryCode::CountryCodeMS)
            {
                output = Json::Value("MS");
                return;
            }
            if (input == CountryCode::CountryCodeMA)
            {
                output = Json::Value("MA");
                return;
            }
            if (input == CountryCode::CountryCodeMZ)
            {
                output = Json::Value("MZ");
                return;
            }
            if (input == CountryCode::CountryCodeMM)
            {
                output = Json::Value("MM");
                return;
            }
            if (input == CountryCode::CountryCodeNA)
            {
                output = Json::Value("NA");
                return;
            }
            if (input == CountryCode::CountryCodeNR)
            {
                output = Json::Value("NR");
                return;
            }
            if (input == CountryCode::CountryCodeNP)
            {
                output = Json::Value("NP");
                return;
            }
            if (input == CountryCode::CountryCodeNL)
            {
                output = Json::Value("NL");
                return;
            }
            if (input == CountryCode::CountryCodeNC)
            {
                output = Json::Value("NC");
                return;
            }
            if (input == CountryCode::CountryCodeNZ)
            {
                output = Json::Value("NZ");
                return;
            }
            if (input == CountryCode::CountryCodeNI)
            {
                output = Json::Value("NI");
                return;
            }
            if (input == CountryCode::CountryCodeNE)
            {
                output = Json::Value("NE");
                return;
            }
            if (input == CountryCode::CountryCodeNG)
            {
                output = Json::Value("NG");
                return;
            }
            if (input == CountryCode::CountryCodeNU)
            {
                output = Json::Value("NU");
                return;
            }
            if (input == CountryCode::CountryCodeNF)
            {
                output = Json::Value("NF");
                return;
            }
            if (input == CountryCode::CountryCodeMP)
            {
                output = Json::Value("MP");
                return;
            }
            if (input == CountryCode::CountryCodeNO)
            {
                output = Json::Value("NO");
                return;
            }
            if (input == CountryCode::CountryCodeOM)
            {
                output = Json::Value("OM");
                return;
            }
            if (input == CountryCode::CountryCodePK)
            {
                output = Json::Value("PK");
                return;
            }
            if (input == CountryCode::CountryCodePW)
            {
                output = Json::Value("PW");
                return;
            }
            if (input == CountryCode::CountryCodePS)
            {
                output = Json::Value("PS");
                return;
            }
            if (input == CountryCode::CountryCodePA)
            {
                output = Json::Value("PA");
                return;
            }
            if (input == CountryCode::CountryCodePG)
            {
                output = Json::Value("PG");
                return;
            }
            if (input == CountryCode::CountryCodePY)
            {
                output = Json::Value("PY");
                return;
            }
            if (input == CountryCode::CountryCodePE)
            {
                output = Json::Value("PE");
                return;
            }
            if (input == CountryCode::CountryCodePH)
            {
                output = Json::Value("PH");
                return;
            }
            if (input == CountryCode::CountryCodePN)
            {
                output = Json::Value("PN");
                return;
            }
            if (input == CountryCode::CountryCodePL)
            {
                output = Json::Value("PL");
                return;
            }
            if (input == CountryCode::CountryCodePT)
            {
                output = Json::Value("PT");
                return;
            }
            if (input == CountryCode::CountryCodePR)
            {
                output = Json::Value("PR");
                return;
            }
            if (input == CountryCode::CountryCodeQA)
            {
                output = Json::Value("QA");
                return;
            }
            if (input == CountryCode::CountryCodeRE)
            {
                output = Json::Value("RE");
                return;
            }
            if (input == CountryCode::CountryCodeRO)
            {
                output = Json::Value("RO");
                return;
            }
            if (input == CountryCode::CountryCodeRU)
            {
                output = Json::Value("RU");
                return;
            }
            if (input == CountryCode::CountryCodeRW)
            {
                output = Json::Value("RW");
                return;
            }
            if (input == CountryCode::CountryCodeBL)
            {
                output = Json::Value("BL");
                return;
            }
            if (input == CountryCode::CountryCodeSH)
            {
                output = Json::Value("SH");
                return;
            }
            if (input == CountryCode::CountryCodeKN)
            {
                output = Json::Value("KN");
                return;
            }
            if (input == CountryCode::CountryCodeLC)
            {
                output = Json::Value("LC");
                return;
            }
            if (input == CountryCode::CountryCodeMF)
            {
                output = Json::Value("MF");
                return;
            }
            if (input == CountryCode::CountryCodePM)
            {
                output = Json::Value("PM");
                return;
            }
            if (input == CountryCode::CountryCodeVC)
            {
                output = Json::Value("VC");
                return;
            }
            if (input == CountryCode::CountryCodeWS)
            {
                output = Json::Value("WS");
                return;
            }
            if (input == CountryCode::CountryCodeSM)
            {
                output = Json::Value("SM");
                return;
            }
            if (input == CountryCode::CountryCodeST)
            {
                output = Json::Value("ST");
                return;
            }
            if (input == CountryCode::CountryCodeSA)
            {
                output = Json::Value("SA");
                return;
            }
            if (input == CountryCode::CountryCodeSN)
            {
                output = Json::Value("SN");
                return;
            }
            if (input == CountryCode::CountryCodeRS)
            {
                output = Json::Value("RS");
                return;
            }
            if (input == CountryCode::CountryCodeSC)
            {
                output = Json::Value("SC");
                return;
            }
            if (input == CountryCode::CountryCodeSL)
            {
                output = Json::Value("SL");
                return;
            }
            if (input == CountryCode::CountryCodeSG)
            {
                output = Json::Value("SG");
                return;
            }
            if (input == CountryCode::CountryCodeSX)
            {
                output = Json::Value("SX");
                return;
            }
            if (input == CountryCode::CountryCodeSK)
            {
                output = Json::Value("SK");
                return;
            }
            if (input == CountryCode::CountryCodeSI)
            {
                output = Json::Value("SI");
                return;
            }
            if (input == CountryCode::CountryCodeSB)
            {
                output = Json::Value("SB");
                return;
            }
            if (input == CountryCode::CountryCodeSO)
            {
                output = Json::Value("SO");
                return;
            }
            if (input == CountryCode::CountryCodeZA)
            {
                output = Json::Value("ZA");
                return;
            }
            if (input == CountryCode::CountryCodeGS)
            {
                output = Json::Value("GS");
                return;
            }
            if (input == CountryCode::CountryCodeSS)
            {
                output = Json::Value("SS");
                return;
            }
            if (input == CountryCode::CountryCodeES)
            {
                output = Json::Value("ES");
                return;
            }
            if (input == CountryCode::CountryCodeLK)
            {
                output = Json::Value("LK");
                return;
            }
            if (input == CountryCode::CountryCodeSD)
            {
                output = Json::Value("SD");
                return;
            }
            if (input == CountryCode::CountryCodeSR)
            {
                output = Json::Value("SR");
                return;
            }
            if (input == CountryCode::CountryCodeSJ)
            {
                output = Json::Value("SJ");
                return;
            }
            if (input == CountryCode::CountryCodeSZ)
            {
                output = Json::Value("SZ");
                return;
            }
            if (input == CountryCode::CountryCodeSE)
            {
                output = Json::Value("SE");
                return;
            }
            if (input == CountryCode::CountryCodeCH)
            {
                output = Json::Value("CH");
                return;
            }
            if (input == CountryCode::CountryCodeSY)
            {
                output = Json::Value("SY");
                return;
            }
            if (input == CountryCode::CountryCodeTW)
            {
                output = Json::Value("TW");
                return;
            }
            if (input == CountryCode::CountryCodeTJ)
            {
                output = Json::Value("TJ");
                return;
            }
            if (input == CountryCode::CountryCodeTZ)
            {
                output = Json::Value("TZ");
                return;
            }
            if (input == CountryCode::CountryCodeTH)
            {
                output = Json::Value("TH");
                return;
            }
            if (input == CountryCode::CountryCodeTL)
            {
                output = Json::Value("TL");
                return;
            }
            if (input == CountryCode::CountryCodeTG)
            {
                output = Json::Value("TG");
                return;
            }
            if (input == CountryCode::CountryCodeTK)
            {
                output = Json::Value("TK");
                return;
            }
            if (input == CountryCode::CountryCodeTO)
            {
                output = Json::Value("TO");
                return;
            }
            if (input == CountryCode::CountryCodeTT)
            {
                output = Json::Value("TT");
                return;
            }
            if (input == CountryCode::CountryCodeTN)
            {
                output = Json::Value("TN");
                return;
            }
            if (input == CountryCode::CountryCodeTR)
            {
                output = Json::Value("TR");
                return;
            }
            if (input == CountryCode::CountryCodeTM)
            {
                output = Json::Value("TM");
                return;
            }
            if (input == CountryCode::CountryCodeTC)
            {
                output = Json::Value("TC");
                return;
            }
            if (input == CountryCode::CountryCodeTV)
            {
                output = Json::Value("TV");
                return;
            }
            if (input == CountryCode::CountryCodeUG)
            {
                output = Json::Value("UG");
                return;
            }
            if (input == CountryCode::CountryCodeUA)
            {
                output = Json::Value("UA");
                return;
            }
            if (input == CountryCode::CountryCodeAE)
            {
                output = Json::Value("AE");
                return;
            }
            if (input == CountryCode::CountryCodeGB)
            {
                output = Json::Value("GB");
                return;
            }
            if (input == CountryCode::CountryCodeUS)
            {
                output = Json::Value("US");
                return;
            }
            if (input == CountryCode::CountryCodeUM)
            {
                output = Json::Value("UM");
                return;
            }
            if (input == CountryCode::CountryCodeUY)
            {
                output = Json::Value("UY");
                return;
            }
            if (input == CountryCode::CountryCodeUZ)
            {
                output = Json::Value("UZ");
                return;
            }
            if (input == CountryCode::CountryCodeVU)
            {
                output = Json::Value("VU");
                return;
            }
            if (input == CountryCode::CountryCodeVE)
            {
                output = Json::Value("VE");
                return;
            }
            if (input == CountryCode::CountryCodeVN)
            {
                output = Json::Value("VN");
                return;
            }
            if (input == CountryCode::CountryCodeVG)
            {
                output = Json::Value("VG");
                return;
            }
            if (input == CountryCode::CountryCodeVI)
            {
                output = Json::Value("VI");
                return;
            }
            if (input == CountryCode::CountryCodeWF)
            {
                output = Json::Value("WF");
                return;
            }
            if (input == CountryCode::CountryCodeEH)
            {
                output = Json::Value("EH");
                return;
            }
            if (input == CountryCode::CountryCodeYE)
            {
                output = Json::Value("YE");
                return;
            }
            if (input == CountryCode::CountryCodeZM)
            {
                output = Json::Value("ZM");
                return;
            }
            if (input == CountryCode::CountryCodeZW)
            {
                output = Json::Value("ZW");
                return;
            }
        }
        inline void FromJsonEnum(const Json::Value& input, CountryCode& output)
        {
            if (!input.isString())
            {
                return;
            }
            const std::string& inputStr = input.asString();
            if (inputStr == "AF")
            {
                output = CountryCode::CountryCodeAF;
                return;
            }
            if (inputStr == "AX")
            {
                output = CountryCode::CountryCodeAX;
                return;
            }
            if (inputStr == "AL")
            {
                output = CountryCode::CountryCodeAL;
                return;
            }
            if (inputStr == "DZ")
            {
                output = CountryCode::CountryCodeDZ;
                return;
            }
            if (inputStr == "AS")
            {
                output = CountryCode::CountryCodeAS;
                return;
            }
            if (inputStr == "AD")
            {
                output = CountryCode::CountryCodeAD;
                return;
            }
            if (inputStr == "AO")
            {
                output = CountryCode::CountryCodeAO;
                return;
            }
            if (inputStr == "AI")
            {
                output = CountryCode::CountryCodeAI;
                return;
            }
            if (inputStr == "AQ")
            {
                output = CountryCode::CountryCodeAQ;
                return;
            }
            if (inputStr == "AG")
            {
                output = CountryCode::CountryCodeAG;
                return;
            }
            if (inputStr == "AR")
            {
                output = CountryCode::CountryCodeAR;
                return;
            }
            if (inputStr == "AM")
            {
                output = CountryCode::CountryCodeAM;
                return;
            }
            if (inputStr == "AW")
            {
                output = CountryCode::CountryCodeAW;
                return;
            }
            if (inputStr == "AU")
            {
                output = CountryCode::CountryCodeAU;
                return;
            }
            if (inputStr == "AT")
            {
                output = CountryCode::CountryCodeAT;
                return;
            }
            if (inputStr == "AZ")
            {
                output = CountryCode::CountryCodeAZ;
                return;
            }
            if (inputStr == "BS")
            {
                output = CountryCode::CountryCodeBS;
                return;
            }
            if (inputStr == "BH")
            {
                output = CountryCode::CountryCodeBH;
                return;
            }
            if (inputStr == "BD")
            {
                output = CountryCode::CountryCodeBD;
                return;
            }
            if (inputStr == "BB")
            {
                output = CountryCode::CountryCodeBB;
                return;
            }
            if (inputStr == "BY")
            {
                output = CountryCode::CountryCodeBY;
                return;
            }
            if (inputStr == "BE")
            {
                output = CountryCode::CountryCodeBE;
                return;
            }
            if (inputStr == "BZ")
            {
                output = CountryCode::CountryCodeBZ;
                return;
            }
            if (inputStr == "BJ")
            {
                output = CountryCode::CountryCodeBJ;
                return;
            }
            if (inputStr == "BM")
            {
                output = CountryCode::CountryCodeBM;
                return;
            }
            if (inputStr == "BT")
            {
                output = CountryCode::CountryCodeBT;
                return;
            }
            if (inputStr == "BO")
            {
                output = CountryCode::CountryCodeBO;
                return;
            }
            if (inputStr == "BQ")
            {
                output = CountryCode::CountryCodeBQ;
                return;
            }
            if (inputStr == "BA")
            {
                output = CountryCode::CountryCodeBA;
                return;
            }
            if (inputStr == "BW")
            {
                output = CountryCode::CountryCodeBW;
                return;
            }
            if (inputStr == "BV")
            {
                output = CountryCode::CountryCodeBV;
                return;
            }
            if (inputStr == "BR")
            {
                output = CountryCode::CountryCodeBR;
                return;
            }
            if (inputStr == "IO")
            {
                output = CountryCode::CountryCodeIO;
                return;
            }
            if (inputStr == "BN")
            {
                output = CountryCode::CountryCodeBN;
                return;
            }
            if (inputStr == "BG")
            {
                output = CountryCode::CountryCodeBG;
                return;
            }
            if (inputStr == "BF")
            {
                output = CountryCode::CountryCodeBF;
                return;
            }
            if (inputStr == "BI")
            {
                output = CountryCode::CountryCodeBI;
                return;
            }
            if (inputStr == "KH")
            {
                output = CountryCode::CountryCodeKH;
                return;
            }
            if (inputStr == "CM")
            {
                output = CountryCode::CountryCodeCM;
                return;
            }
            if (inputStr == "CA")
            {
                output = CountryCode::CountryCodeCA;
                return;
            }
            if (inputStr == "CV")
            {
                output = CountryCode::CountryCodeCV;
                return;
            }
            if (inputStr == "KY")
            {
                output = CountryCode::CountryCodeKY;
                return;
            }
            if (inputStr == "CF")
            {
                output = CountryCode::CountryCodeCF;
                return;
            }
            if (inputStr == "TD")
            {
                output = CountryCode::CountryCodeTD;
                return;
            }
            if (inputStr == "CL")
            {
                output = CountryCode::CountryCodeCL;
                return;
            }
            if (inputStr == "CN")
            {
                output = CountryCode::CountryCodeCN;
                return;
            }
            if (inputStr == "CX")
            {
                output = CountryCode::CountryCodeCX;
                return;
            }
            if (inputStr == "CC")
            {
                output = CountryCode::CountryCodeCC;
                return;
            }
            if (inputStr == "CO")
            {
                output = CountryCode::CountryCodeCO;
                return;
            }
            if (inputStr == "KM")
            {
                output = CountryCode::CountryCodeKM;
                return;
            }
            if (inputStr == "CG")
            {
                output = CountryCode::CountryCodeCG;
                return;
            }
            if (inputStr == "CD")
            {
                output = CountryCode::CountryCodeCD;
                return;
            }
            if (inputStr == "CK")
            {
                output = CountryCode::CountryCodeCK;
                return;
            }
            if (inputStr == "CR")
            {
                output = CountryCode::CountryCodeCR;
                return;
            }
            if (inputStr == "CI")
            {
                output = CountryCode::CountryCodeCI;
                return;
            }
            if (inputStr == "HR")
            {
                output = CountryCode::CountryCodeHR;
                return;
            }
            if (inputStr == "CU")
            {
                output = CountryCode::CountryCodeCU;
                return;
            }
            if (inputStr == "CW")
            {
                output = CountryCode::CountryCodeCW;
                return;
            }
            if (inputStr == "CY")
            {
                output = CountryCode::CountryCodeCY;
                return;
            }
            if (inputStr == "CZ")
            {
                output = CountryCode::CountryCodeCZ;
                return;
            }
            if (inputStr == "DK")
            {
                output = CountryCode::CountryCodeDK;
                return;
            }
            if (inputStr == "DJ")
            {
                output = CountryCode::CountryCodeDJ;
                return;
            }
            if (inputStr == "DM")
            {
                output = CountryCode::CountryCodeDM;
                return;
            }
            if (inputStr == "DO")
            {
                output = CountryCode::CountryCodeDO;
                return;
            }
            if (inputStr == "EC")
            {
                output = CountryCode::CountryCodeEC;
                return;
            }
            if (inputStr == "EG")
            {
                output = CountryCode::CountryCodeEG;
                return;
            }
            if (inputStr == "SV")
            {
                output = CountryCode::CountryCodeSV;
                return;
            }
            if (inputStr == "GQ")
            {
                output = CountryCode::CountryCodeGQ;
                return;
            }
            if (inputStr == "ER")
            {
                output = CountryCode::CountryCodeER;
                return;
            }
            if (inputStr == "EE")
            {
                output = CountryCode::CountryCodeEE;
                return;
            }
            if (inputStr == "ET")
            {
                output = CountryCode::CountryCodeET;
                return;
            }
            if (inputStr == "FK")
            {
                output = CountryCode::CountryCodeFK;
                return;
            }
            if (inputStr == "FO")
            {
                output = CountryCode::CountryCodeFO;
                return;
            }
            if (inputStr == "FJ")
            {
                output = CountryCode::CountryCodeFJ;
                return;
            }
            if (inputStr == "FI")
            {
                output = CountryCode::CountryCodeFI;
                return;
            }
            if (inputStr == "FR")
            {
                output = CountryCode::CountryCodeFR;
                return;
            }
            if (inputStr == "GF")
            {
                output = CountryCode::CountryCodeGF;
                return;
            }
            if (inputStr == "PF")
            {
                output = CountryCode::CountryCodePF;
                return;
            }
            if (inputStr == "TF")
            {
                output = CountryCode::CountryCodeTF;
                return;
            }
            if (inputStr == "GA")
            {
                output = CountryCode::CountryCodeGA;
                return;
            }
            if (inputStr == "GM")
            {
                output = CountryCode::CountryCodeGM;
                return;
            }
            if (inputStr == "GE")
            {
                output = CountryCode::CountryCodeGE;
                return;
            }
            if (inputStr == "DE")
            {
                output = CountryCode::CountryCodeDE;
                return;
            }
            if (inputStr == "GH")
            {
                output = CountryCode::CountryCodeGH;
                return;
            }
            if (inputStr == "GI")
            {
                output = CountryCode::CountryCodeGI;
                return;
            }
            if (inputStr == "GR")
            {
                output = CountryCode::CountryCodeGR;
                return;
            }
            if (inputStr == "GL")
            {
                output = CountryCode::CountryCodeGL;
                return;
            }
            if (inputStr == "GD")
            {
                output = CountryCode::CountryCodeGD;
                return;
            }
            if (inputStr == "GP")
            {
                output = CountryCode::CountryCodeGP;
                return;
            }
            if (inputStr == "GU")
            {
                output = CountryCode::CountryCodeGU;
                return;
            }
            if (inputStr == "GT")
            {
                output = CountryCode::CountryCodeGT;
                return;
            }
            if (inputStr == "GG")
            {
                output = CountryCode::CountryCodeGG;
                return;
            }
            if (inputStr == "GN")
            {
                output = CountryCode::CountryCodeGN;
                return;
            }
            if (inputStr == "GW")
            {
                output = CountryCode::CountryCodeGW;
                return;
            }
            if (inputStr == "GY")
            {
                output = CountryCode::CountryCodeGY;
                return;
            }
            if (inputStr == "HT")
            {
                output = CountryCode::CountryCodeHT;
                return;
            }
            if (inputStr == "HM")
            {
                output = CountryCode::CountryCodeHM;
                return;
            }
            if (inputStr == "VA")
            {
                output = CountryCode::CountryCodeVA;
                return;
            }
            if (inputStr == "HN")
            {
                output = CountryCode::CountryCodeHN;
                return;
            }
            if (inputStr == "HK")
            {
                output = CountryCode::CountryCodeHK;
                return;
            }
            if (inputStr == "HU")
            {
                output = CountryCode::CountryCodeHU;
                return;
            }
            if (inputStr == "IS")
            {
                output = CountryCode::CountryCodeIS;
                return;
            }
            if (inputStr == "IN")
            {
                output = CountryCode::CountryCodeIN;
                return;
            }
            if (inputStr == "ID")
            {
                output = CountryCode::CountryCodeID;
                return;
            }
            if (inputStr == "IR")
            {
                output = CountryCode::CountryCodeIR;
                return;
            }
            if (inputStr == "IQ")
            {
                output = CountryCode::CountryCodeIQ;
                return;
            }
            if (inputStr == "IE")
            {
                output = CountryCode::CountryCodeIE;
                return;
            }
            if (inputStr == "IM")
            {
                output = CountryCode::CountryCodeIM;
                return;
            }
            if (inputStr == "IL")
            {
                output = CountryCode::CountryCodeIL;
                return;
            }
            if (inputStr == "IT")
            {
                output = CountryCode::CountryCodeIT;
                return;
            }
            if (inputStr == "JM")
            {
                output = CountryCode::CountryCodeJM;
                return;
            }
            if (inputStr == "JP")
            {
                output = CountryCode::CountryCodeJP;
                return;
            }
            if (inputStr == "JE")
            {
                output = CountryCode::CountryCodeJE;
                return;
            }
            if (inputStr == "JO")
            {
                output = CountryCode::CountryCodeJO;
                return;
            }
            if (inputStr == "KZ")
            {
                output = CountryCode::CountryCodeKZ;
                return;
            }
            if (inputStr == "KE")
            {
                output = CountryCode::CountryCodeKE;
                return;
            }
            if (inputStr == "KI")
            {
                output = CountryCode::CountryCodeKI;
                return;
            }
            if (inputStr == "KP")
            {
                output = CountryCode::CountryCodeKP;
                return;
            }
            if (inputStr == "KR")
            {
                output = CountryCode::CountryCodeKR;
                return;
            }
            if (inputStr == "KW")
            {
                output = CountryCode::CountryCodeKW;
                return;
            }
            if (inputStr == "KG")
            {
                output = CountryCode::CountryCodeKG;
                return;
            }
            if (inputStr == "LA")
            {
                output = CountryCode::CountryCodeLA;
                return;
            }
            if (inputStr == "LV")
            {
                output = CountryCode::CountryCodeLV;
                return;
            }
            if (inputStr == "LB")
            {
                output = CountryCode::CountryCodeLB;
                return;
            }
            if (inputStr == "LS")
            {
                output = CountryCode::CountryCodeLS;
                return;
            }
            if (inputStr == "LR")
            {
                output = CountryCode::CountryCodeLR;
                return;
            }
            if (inputStr == "LY")
            {
                output = CountryCode::CountryCodeLY;
                return;
            }
            if (inputStr == "LI")
            {
                output = CountryCode::CountryCodeLI;
                return;
            }
            if (inputStr == "LT")
            {
                output = CountryCode::CountryCodeLT;
                return;
            }
            if (inputStr == "LU")
            {
                output = CountryCode::CountryCodeLU;
                return;
            }
            if (inputStr == "MO")
            {
                output = CountryCode::CountryCodeMO;
                return;
            }
            if (inputStr == "MK")
            {
                output = CountryCode::CountryCodeMK;
                return;
            }
            if (inputStr == "MG")
            {
                output = CountryCode::CountryCodeMG;
                return;
            }
            if (inputStr == "MW")
            {
                output = CountryCode::CountryCodeMW;
                return;
            }
            if (inputStr == "MY")
            {
                output = CountryCode::CountryCodeMY;
                return;
            }
            if (inputStr == "MV")
            {
                output = CountryCode::CountryCodeMV;
                return;
            }
            if (inputStr == "ML")
            {
                output = CountryCode::CountryCodeML;
                return;
            }
            if (inputStr == "MT")
            {
                output = CountryCode::CountryCodeMT;
                return;
            }
            if (inputStr == "MH")
            {
                output = CountryCode::CountryCodeMH;
                return;
            }
            if (inputStr == "MQ")
            {
                output = CountryCode::CountryCodeMQ;
                return;
            }
            if (inputStr == "MR")
            {
                output = CountryCode::CountryCodeMR;
                return;
            }
            if (inputStr == "MU")
            {
                output = CountryCode::CountryCodeMU;
                return;
            }
            if (inputStr == "YT")
            {
                output = CountryCode::CountryCodeYT;
                return;
            }
            if (inputStr == "MX")
            {
                output = CountryCode::CountryCodeMX;
                return;
            }
            if (inputStr == "FM")
            {
                output = CountryCode::CountryCodeFM;
                return;
            }
            if (inputStr == "MD")
            {
                output = CountryCode::CountryCodeMD;
                return;
            }
            if (inputStr == "MC")
            {
                output = CountryCode::CountryCodeMC;
                return;
            }
            if (inputStr == "MN")
            {
                output = CountryCode::CountryCodeMN;
                return;
            }
            if (inputStr == "ME")
            {
                output = CountryCode::CountryCodeME;
                return;
            }
            if (inputStr == "MS")
            {
                output = CountryCode::CountryCodeMS;
                return;
            }
            if (inputStr == "MA")
            {
                output = CountryCode::CountryCodeMA;
                return;
            }
            if (inputStr == "MZ")
            {
                output = CountryCode::CountryCodeMZ;
                return;
            }
            if (inputStr == "MM")
            {
                output = CountryCode::CountryCodeMM;
                return;
            }
            if (inputStr == "NA")
            {
                output = CountryCode::CountryCodeNA;
                return;
            }
            if (inputStr == "NR")
            {
                output = CountryCode::CountryCodeNR;
                return;
            }
            if (inputStr == "NP")
            {
                output = CountryCode::CountryCodeNP;
                return;
            }
            if (inputStr == "NL")
            {
                output = CountryCode::CountryCodeNL;
                return;
            }
            if (inputStr == "NC")
            {
                output = CountryCode::CountryCodeNC;
                return;
            }
            if (inputStr == "NZ")
            {
                output = CountryCode::CountryCodeNZ;
                return;
            }
            if (inputStr == "NI")
            {
                output = CountryCode::CountryCodeNI;
                return;
            }
            if (inputStr == "NE")
            {
                output = CountryCode::CountryCodeNE;
                return;
            }
            if (inputStr == "NG")
            {
                output = CountryCode::CountryCodeNG;
                return;
            }
            if (inputStr == "NU")
            {
                output = CountryCode::CountryCodeNU;
                return;
            }
            if (inputStr == "NF")
            {
                output = CountryCode::CountryCodeNF;
                return;
            }
            if (inputStr == "MP")
            {
                output = CountryCode::CountryCodeMP;
                return;
            }
            if (inputStr == "NO")
            {
                output = CountryCode::CountryCodeNO;
                return;
            }
            if (inputStr == "OM")
            {
                output = CountryCode::CountryCodeOM;
                return;
            }
            if (inputStr == "PK")
            {
                output = CountryCode::CountryCodePK;
                return;
            }
            if (inputStr == "PW")
            {
                output = CountryCode::CountryCodePW;
                return;
            }
            if (inputStr == "PS")
            {
                output = CountryCode::CountryCodePS;
                return;
            }
            if (inputStr == "PA")
            {
                output = CountryCode::CountryCodePA;
                return;
            }
            if (inputStr == "PG")
            {
                output = CountryCode::CountryCodePG;
                return;
            }
            if (inputStr == "PY")
            {
                output = CountryCode::CountryCodePY;
                return;
            }
            if (inputStr == "PE")
            {
                output = CountryCode::CountryCodePE;
                return;
            }
            if (inputStr == "PH")
            {
                output = CountryCode::CountryCodePH;
                return;
            }
            if (inputStr == "PN")
            {
                output = CountryCode::CountryCodePN;
                return;
            }
            if (inputStr == "PL")
            {
                output = CountryCode::CountryCodePL;
                return;
            }
            if (inputStr == "PT")
            {
                output = CountryCode::CountryCodePT;
                return;
            }
            if (inputStr == "PR")
            {
                output = CountryCode::CountryCodePR;
                return;
            }
            if (inputStr == "QA")
            {
                output = CountryCode::CountryCodeQA;
                return;
            }
            if (inputStr == "RE")
            {
                output = CountryCode::CountryCodeRE;
                return;
            }
            if (inputStr == "RO")
            {
                output = CountryCode::CountryCodeRO;
                return;
            }
            if (inputStr == "RU")
            {
                output = CountryCode::CountryCodeRU;
                return;
            }
            if (inputStr == "RW")
            {
                output = CountryCode::CountryCodeRW;
                return;
            }
            if (inputStr == "BL")
            {
                output = CountryCode::CountryCodeBL;
                return;
            }
            if (inputStr == "SH")
            {
                output = CountryCode::CountryCodeSH;
                return;
            }
            if (inputStr == "KN")
            {
                output = CountryCode::CountryCodeKN;
                return;
            }
            if (inputStr == "LC")
            {
                output = CountryCode::CountryCodeLC;
                return;
            }
            if (inputStr == "MF")
            {
                output = CountryCode::CountryCodeMF;
                return;
            }
            if (inputStr == "PM")
            {
                output = CountryCode::CountryCodePM;
                return;
            }
            if (inputStr == "VC")
            {
                output = CountryCode::CountryCodeVC;
                return;
            }
            if (inputStr == "WS")
            {
                output = CountryCode::CountryCodeWS;
                return;
            }
            if (inputStr == "SM")
            {
                output = CountryCode::CountryCodeSM;
                return;
            }
            if (inputStr == "ST")
            {
                output = CountryCode::CountryCodeST;
                return;
            }
            if (inputStr == "SA")
            {
                output = CountryCode::CountryCodeSA;
                return;
            }
            if (inputStr == "SN")
            {
                output = CountryCode::CountryCodeSN;
                return;
            }
            if (inputStr == "RS")
            {
                output = CountryCode::CountryCodeRS;
                return;
            }
            if (inputStr == "SC")
            {
                output = CountryCode::CountryCodeSC;
                return;
            }
            if (inputStr == "SL")
            {
                output = CountryCode::CountryCodeSL;
                return;
            }
            if (inputStr == "SG")
            {
                output = CountryCode::CountryCodeSG;
                return;
            }
            if (inputStr == "SX")
            {
                output = CountryCode::CountryCodeSX;
                return;
            }
            if (inputStr == "SK")
            {
                output = CountryCode::CountryCodeSK;
                return;
            }
            if (inputStr == "SI")
            {
                output = CountryCode::CountryCodeSI;
                return;
            }
            if (inputStr == "SB")
            {
                output = CountryCode::CountryCodeSB;
                return;
            }
            if (inputStr == "SO")
            {
                output = CountryCode::CountryCodeSO;
                return;
            }
            if (inputStr == "ZA")
            {
                output = CountryCode::CountryCodeZA;
                return;
            }
            if (inputStr == "GS")
            {
                output = CountryCode::CountryCodeGS;
                return;
            }
            if (inputStr == "SS")
            {
                output = CountryCode::CountryCodeSS;
                return;
            }
            if (inputStr == "ES")
            {
                output = CountryCode::CountryCodeES;
                return;
            }
            if (inputStr == "LK")
            {
                output = CountryCode::CountryCodeLK;
                return;
            }
            if (inputStr == "SD")
            {
                output = CountryCode::CountryCodeSD;
                return;
            }
            if (inputStr == "SR")
            {
                output = CountryCode::CountryCodeSR;
                return;
            }
            if (inputStr == "SJ")
            {
                output = CountryCode::CountryCodeSJ;
                return;
            }
            if (inputStr == "SZ")
            {
                output = CountryCode::CountryCodeSZ;
                return;
            }
            if (inputStr == "SE")
            {
                output = CountryCode::CountryCodeSE;
                return;
            }
            if (inputStr == "CH")
            {
                output = CountryCode::CountryCodeCH;
                return;
            }
            if (inputStr == "SY")
            {
                output = CountryCode::CountryCodeSY;
                return;
            }
            if (inputStr == "TW")
            {
                output = CountryCode::CountryCodeTW;
                return;
            }
            if (inputStr == "TJ")
            {
                output = CountryCode::CountryCodeTJ;
                return;
            }
            if (inputStr == "TZ")
            {
                output = CountryCode::CountryCodeTZ;
                return;
            }
            if (inputStr == "TH")
            {
                output = CountryCode::CountryCodeTH;
                return;
            }
            if (inputStr == "TL")
            {
                output = CountryCode::CountryCodeTL;
                return;
            }
            if (inputStr == "TG")
            {
                output = CountryCode::CountryCodeTG;
                return;
            }
            if (inputStr == "TK")
            {
                output = CountryCode::CountryCodeTK;
                return;
            }
            if (inputStr == "TO")
            {
                output = CountryCode::CountryCodeTO;
                return;
            }
            if (inputStr == "TT")
            {
                output = CountryCode::CountryCodeTT;
                return;
            }
            if (inputStr == "TN")
            {
                output = CountryCode::CountryCodeTN;
                return;
            }
            if (inputStr == "TR")
            {
                output = CountryCode::CountryCodeTR;
                return;
            }
            if (inputStr == "TM")
            {
                output = CountryCode::CountryCodeTM;
                return;
            }
            if (inputStr == "TC")
            {
                output = CountryCode::CountryCodeTC;
                return;
            }
            if (inputStr == "TV")
            {
                output = CountryCode::CountryCodeTV;
                return;
            }
            if (inputStr == "UG")
            {
                output = CountryCode::CountryCodeUG;
                return;
            }
            if (inputStr == "UA")
            {
                output = CountryCode::CountryCodeUA;
                return;
            }
            if (inputStr == "AE")
            {
                output = CountryCode::CountryCodeAE;
                return;
            }
            if (inputStr == "GB")
            {
                output = CountryCode::CountryCodeGB;
                return;
            }
            if (inputStr == "US")
            {
                output = CountryCode::CountryCodeUS;
                return;
            }
            if (inputStr == "UM")
            {
                output = CountryCode::CountryCodeUM;
                return;
            }
            if (inputStr == "UY")
            {
                output = CountryCode::CountryCodeUY;
                return;
            }
            if (inputStr == "UZ")
            {
                output = CountryCode::CountryCodeUZ;
                return;
            }
            if (inputStr == "VU")
            {
                output = CountryCode::CountryCodeVU;
                return;
            }
            if (inputStr == "VE")
            {
                output = CountryCode::CountryCodeVE;
                return;
            }
            if (inputStr == "VN")
            {
                output = CountryCode::CountryCodeVN;
                return;
            }
            if (inputStr == "VG")
            {
                output = CountryCode::CountryCodeVG;
                return;
            }
            if (inputStr == "VI")
            {
                output = CountryCode::CountryCodeVI;
                return;
            }
            if (inputStr == "WF")
            {
                output = CountryCode::CountryCodeWF;
                return;
            }
            if (inputStr == "EH")
            {
                output = CountryCode::CountryCodeEH;
                return;
            }
            if (inputStr == "YE")
            {
                output = CountryCode::CountryCodeYE;
                return;
            }
            if (inputStr == "ZM")
            {
                output = CountryCode::CountryCodeZM;
                return;
            }
            if (inputStr == "ZW")
            {
                output = CountryCode::CountryCodeZW;
                return;
            }
        }

        enum class Currency
        {
            CurrencyAED,
            CurrencyAFN,
            CurrencyALL,
            CurrencyAMD,
            CurrencyANG,
            CurrencyAOA,
            CurrencyARS,
            CurrencyAUD,
            CurrencyAWG,
            CurrencyAZN,
            CurrencyBAM,
            CurrencyBBD,
            CurrencyBDT,
            CurrencyBGN,
            CurrencyBHD,
            CurrencyBIF,
            CurrencyBMD,
            CurrencyBND,
            CurrencyBOB,
            CurrencyBRL,
            CurrencyBSD,
            CurrencyBTN,
            CurrencyBWP,
            CurrencyBYR,
            CurrencyBZD,
            CurrencyCAD,
            CurrencyCDF,
            CurrencyCHF,
            CurrencyCLP,
            CurrencyCNY,
            CurrencyCOP,
            CurrencyCRC,
            CurrencyCUC,
            CurrencyCUP,
            CurrencyCVE,
            CurrencyCZK,
            CurrencyDJF,
            CurrencyDKK,
            CurrencyDOP,
            CurrencyDZD,
            CurrencyEGP,
            CurrencyERN,
            CurrencyETB,
            CurrencyEUR,
            CurrencyFJD,
            CurrencyFKP,
            CurrencyGBP,
            CurrencyGEL,
            CurrencyGGP,
            CurrencyGHS,
            CurrencyGIP,
            CurrencyGMD,
            CurrencyGNF,
            CurrencyGTQ,
            CurrencyGYD,
            CurrencyHKD,
            CurrencyHNL,
            CurrencyHRK,
            CurrencyHTG,
            CurrencyHUF,
            CurrencyIDR,
            CurrencyILS,
            CurrencyIMP,
            CurrencyINR,
            CurrencyIQD,
            CurrencyIRR,
            CurrencyISK,
            CurrencyJEP,
            CurrencyJMD,
            CurrencyJOD,
            CurrencyJPY,
            CurrencyKES,
            CurrencyKGS,
            CurrencyKHR,
            CurrencyKMF,
            CurrencyKPW,
            CurrencyKRW,
            CurrencyKWD,
            CurrencyKYD,
            CurrencyKZT,
            CurrencyLAK,
            CurrencyLBP,
            CurrencyLKR,
            CurrencyLRD,
            CurrencyLSL,
            CurrencyLYD,
            CurrencyMAD,
            CurrencyMDL,
            CurrencyMGA,
            CurrencyMKD,
            CurrencyMMK,
            CurrencyMNT,
            CurrencyMOP,
            CurrencyMRO,
            CurrencyMUR,
            CurrencyMVR,
            CurrencyMWK,
            CurrencyMXN,
            CurrencyMYR,
            CurrencyMZN,
            CurrencyNAD,
            CurrencyNGN,
            CurrencyNIO,
            CurrencyNOK,
            CurrencyNPR,
            CurrencyNZD,
            CurrencyOMR,
            CurrencyPAB,
            CurrencyPEN,
            CurrencyPGK,
            CurrencyPHP,
            CurrencyPKR,
            CurrencyPLN,
            CurrencyPYG,
            CurrencyQAR,
            CurrencyRON,
            CurrencyRSD,
            CurrencyRUB,
            CurrencyRWF,
            CurrencySAR,
            CurrencySBD,
            CurrencySCR,
            CurrencySDG,
            CurrencySEK,
            CurrencySGD,
            CurrencySHP,
            CurrencySLL,
            CurrencySOS,
            CurrencySPL,
            CurrencySRD,
            CurrencySTD,
            CurrencySVC,
            CurrencySYP,
            CurrencySZL,
            CurrencyTHB,
            CurrencyTJS,
            CurrencyTMT,
            CurrencyTND,
            CurrencyTOP,
            CurrencyTRY,
            CurrencyTTD,
            CurrencyTVD,
            CurrencyTWD,
            CurrencyTZS,
            CurrencyUAH,
            CurrencyUGX,
            CurrencyUSD,
            CurrencyUYU,
            CurrencyUZS,
            CurrencyVEF,
            CurrencyVND,
            CurrencyVUV,
            CurrencyWST,
            CurrencyXAF,
            CurrencyXCD,
            CurrencyXDR,
            CurrencyXOF,
            CurrencyXPF,
            CurrencyYER,
            CurrencyZAR,
            CurrencyZMW,
            CurrencyZWD
        };

        inline void ToJsonEnum(const Currency input, Json::Value& output)
        {
            if (input == Currency::CurrencyAED)
            {
                output = Json::Value("AED");
                return;
            }
            if (input == Currency::CurrencyAFN)
            {
                output = Json::Value("AFN");
                return;
            }
            if (input == Currency::CurrencyALL)
            {
                output = Json::Value("ALL");
                return;
            }
            if (input == Currency::CurrencyAMD)
            {
                output = Json::Value("AMD");
                return;
            }
            if (input == Currency::CurrencyANG)
            {
                output = Json::Value("ANG");
                return;
            }
            if (input == Currency::CurrencyAOA)
            {
                output = Json::Value("AOA");
                return;
            }
            if (input == Currency::CurrencyARS)
            {
                output = Json::Value("ARS");
                return;
            }
            if (input == Currency::CurrencyAUD)
            {
                output = Json::Value("AUD");
                return;
            }
            if (input == Currency::CurrencyAWG)
            {
                output = Json::Value("AWG");
                return;
            }
            if (input == Currency::CurrencyAZN)
            {
                output = Json::Value("AZN");
                return;
            }
            if (input == Currency::CurrencyBAM)
            {
                output = Json::Value("BAM");
                return;
            }
            if (input == Currency::CurrencyBBD)
            {
                output = Json::Value("BBD");
                return;
            }
            if (input == Currency::CurrencyBDT)
            {
                output = Json::Value("BDT");
                return;
            }
            if (input == Currency::CurrencyBGN)
            {
                output = Json::Value("BGN");
                return;
            }
            if (input == Currency::CurrencyBHD)
            {
                output = Json::Value("BHD");
                return;
            }
            if (input == Currency::CurrencyBIF)
            {
                output = Json::Value("BIF");
                return;
            }
            if (input == Currency::CurrencyBMD)
            {
                output = Json::Value("BMD");
                return;
            }
            if (input == Currency::CurrencyBND)
            {
                output = Json::Value("BND");
                return;
            }
            if (input == Currency::CurrencyBOB)
            {
                output = Json::Value("BOB");
                return;
            }
            if (input == Currency::CurrencyBRL)
            {
                output = Json::Value("BRL");
                return;
            }
            if (input == Currency::CurrencyBSD)
            {
                output = Json::Value("BSD");
                return;
            }
            if (input == Currency::CurrencyBTN)
            {
                output = Json::Value("BTN");
                return;
            }
            if (input == Currency::CurrencyBWP)
            {
                output = Json::Value("BWP");
                return;
            }
            if (input == Currency::CurrencyBYR)
            {
                output = Json::Value("BYR");
                return;
            }
            if (input == Currency::CurrencyBZD)
            {
                output = Json::Value("BZD");
                return;
            }
            if (input == Currency::CurrencyCAD)
            {
                output = Json::Value("CAD");
                return;
            }
            if (input == Currency::CurrencyCDF)
            {
                output = Json::Value("CDF");
                return;
            }
            if (input == Currency::CurrencyCHF)
            {
                output = Json::Value("CHF");
                return;
            }
            if (input == Currency::CurrencyCLP)
            {
                output = Json::Value("CLP");
                return;
            }
            if (input == Currency::CurrencyCNY)
            {
                output = Json::Value("CNY");
                return;
            }
            if (input == Currency::CurrencyCOP)
            {
                output = Json::Value("COP");
                return;
            }
            if (input == Currency::CurrencyCRC)
            {
                output = Json::Value("CRC");
                return;
            }
            if (input == Currency::CurrencyCUC)
            {
                output = Json::Value("CUC");
                return;
            }
            if (input == Currency::CurrencyCUP)
            {
                output = Json::Value("CUP");
                return;
            }
            if (input == Currency::CurrencyCVE)
            {
                output = Json::Value("CVE");
                return;
            }
            if (input == Currency::CurrencyCZK)
            {
                output = Json::Value("CZK");
                return;
            }
            if (input == Currency::CurrencyDJF)
            {
                output = Json::Value("DJF");
                return;
            }
            if (input == Currency::CurrencyDKK)
            {
                output = Json::Value("DKK");
                return;
            }
            if (input == Currency::CurrencyDOP)
            {
                output = Json::Value("DOP");
                return;
            }
            if (input == Currency::CurrencyDZD)
            {
                output = Json::Value("DZD");
                return;
            }
            if (input == Currency::CurrencyEGP)
            {
                output = Json::Value("EGP");
                return;
            }
            if (input == Currency::CurrencyERN)
            {
                output = Json::Value("ERN");
                return;
            }
            if (input == Currency::CurrencyETB)
            {
                output = Json::Value("ETB");
                return;
            }
            if (input == Currency::CurrencyEUR)
            {
                output = Json::Value("EUR");
                return;
            }
            if (input == Currency::CurrencyFJD)
            {
                output = Json::Value("FJD");
                return;
            }
            if (input == Currency::CurrencyFKP)
            {
                output = Json::Value("FKP");
                return;
            }
            if (input == Currency::CurrencyGBP)
            {
                output = Json::Value("GBP");
                return;
            }
            if (input == Currency::CurrencyGEL)
            {
                output = Json::Value("GEL");
                return;
            }
            if (input == Currency::CurrencyGGP)
            {
                output = Json::Value("GGP");
                return;
            }
            if (input == Currency::CurrencyGHS)
            {
                output = Json::Value("GHS");
                return;
            }
            if (input == Currency::CurrencyGIP)
            {
                output = Json::Value("GIP");
                return;
            }
            if (input == Currency::CurrencyGMD)
            {
                output = Json::Value("GMD");
                return;
            }
            if (input == Currency::CurrencyGNF)
            {
                output = Json::Value("GNF");
                return;
            }
            if (input == Currency::CurrencyGTQ)
            {
                output = Json::Value("GTQ");
                return;
            }
            if (input == Currency::CurrencyGYD)
            {
                output = Json::Value("GYD");
                return;
            }
            if (input == Currency::CurrencyHKD)
            {
                output = Json::Value("HKD");
                return;
            }
            if (input == Currency::CurrencyHNL)
            {
                output = Json::Value("HNL");
                return;
            }
            if (input == Currency::CurrencyHRK)
            {
                output = Json::Value("HRK");
                return;
            }
            if (input == Currency::CurrencyHTG)
            {
                output = Json::Value("HTG");
                return;
            }
            if (input == Currency::CurrencyHUF)
            {
                output = Json::Value("HUF");
                return;
            }
            if (input == Currency::CurrencyIDR)
            {
                output = Json::Value("IDR");
                return;
            }
            if (input == Currency::CurrencyILS)
            {
                output = Json::Value("ILS");
                return;
            }
            if (input == Currency::CurrencyIMP)
            {
                output = Json::Value("IMP");
                return;
            }
            if (input == Currency::CurrencyINR)
            {
                output = Json::Value("INR");
                return;
            }
            if (input == Currency::CurrencyIQD)
            {
                output = Json::Value("IQD");
                return;
            }
            if (input == Currency::CurrencyIRR)
            {
                output = Json::Value("IRR");
                return;
            }
            if (input == Currency::CurrencyISK)
            {
                output = Json::Value("ISK");
                return;
            }
            if (input == Currency::CurrencyJEP)
            {
                output = Json::Value("JEP");
                return;
            }
            if (input == Currency::CurrencyJMD)
            {
                output = Json::Value("JMD");
                return;
            }
            if (input == Currency::CurrencyJOD)
            {
                output = Json::Value("JOD");
                return;
            }
            if (input == Currency::CurrencyJPY)
            {
                output = Json::Value("JPY");
                return;
            }
            if (input == Currency::CurrencyKES)
            {
                output = Json::Value("KES");
                return;
            }
            if (input == Currency::CurrencyKGS)
            {
                output = Json::Value("KGS");
                return;
            }
            if (input == Currency::CurrencyKHR)
            {
                output = Json::Value("KHR");
                return;
            }
            if (input == Currency::CurrencyKMF)
            {
                output = Json::Value("KMF");
                return;
            }
            if (input == Currency::CurrencyKPW)
            {
                output = Json::Value("KPW");
                return;
            }
            if (input == Currency::CurrencyKRW)
            {
                output = Json::Value("KRW");
                return;
            }
            if (input == Currency::CurrencyKWD)
            {
                output = Json::Value("KWD");
                return;
            }
            if (input == Currency::CurrencyKYD)
            {
                output = Json::Value("KYD");
                return;
            }
            if (input == Currency::CurrencyKZT)
            {
                output = Json::Value("KZT");
                return;
            }
            if (input == Currency::CurrencyLAK)
            {
                output = Json::Value("LAK");
                return;
            }
            if (input == Currency::CurrencyLBP)
            {
                output = Json::Value("LBP");
                return;
            }
            if (input == Currency::CurrencyLKR)
            {
                output = Json::Value("LKR");
                return;
            }
            if (input == Currency::CurrencyLRD)
            {
                output = Json::Value("LRD");
                return;
            }
            if (input == Currency::CurrencyLSL)
            {
                output = Json::Value("LSL");
                return;
            }
            if (input == Currency::CurrencyLYD)
            {
                output = Json::Value("LYD");
                return;
            }
            if (input == Currency::CurrencyMAD)
            {
                output = Json::Value("MAD");
                return;
            }
            if (input == Currency::CurrencyMDL)
            {
                output = Json::Value("MDL");
                return;
            }
            if (input == Currency::CurrencyMGA)
            {
                output = Json::Value("MGA");
                return;
            }
            if (input == Currency::CurrencyMKD)
            {
                output = Json::Value("MKD");
                return;
            }
            if (input == Currency::CurrencyMMK)
            {
                output = Json::Value("MMK");
                return;
            }
            if (input == Currency::CurrencyMNT)
            {
                output = Json::Value("MNT");
                return;
            }
            if (input == Currency::CurrencyMOP)
            {
                output = Json::Value("MOP");
                return;
            }
            if (input == Currency::CurrencyMRO)
            {
                output = Json::Value("MRO");
                return;
            }
            if (input == Currency::CurrencyMUR)
            {
                output = Json::Value("MUR");
                return;
            }
            if (input == Currency::CurrencyMVR)
            {
                output = Json::Value("MVR");
                return;
            }
            if (input == Currency::CurrencyMWK)
            {
                output = Json::Value("MWK");
                return;
            }
            if (input == Currency::CurrencyMXN)
            {
                output = Json::Value("MXN");
                return;
            }
            if (input == Currency::CurrencyMYR)
            {
                output = Json::Value("MYR");
                return;
            }
            if (input == Currency::CurrencyMZN)
            {
                output = Json::Value("MZN");
                return;
            }
            if (input == Currency::CurrencyNAD)
            {
                output = Json::Value("NAD");
                return;
            }
            if (input == Currency::CurrencyNGN)
            {
                output = Json::Value("NGN");
                return;
            }
            if (input == Currency::CurrencyNIO)
            {
                output = Json::Value("NIO");
                return;
            }
            if (input == Currency::CurrencyNOK)
            {
                output = Json::Value("NOK");
                return;
            }
            if (input == Currency::CurrencyNPR)
            {
                output = Json::Value("NPR");
                return;
            }
            if (input == Currency::CurrencyNZD)
            {
                output = Json::Value("NZD");
                return;
            }
            if (input == Currency::CurrencyOMR)
            {
                output = Json::Value("OMR");
                return;
            }
            if (input == Currency::CurrencyPAB)
            {
                output = Json::Value("PAB");
                return;
            }
            if (input == Currency::CurrencyPEN)
            {
                output = Json::Value("PEN");
                return;
            }
            if (input == Currency::CurrencyPGK)
            {
                output = Json::Value("PGK");
                return;
            }
            if (input == Currency::CurrencyPHP)
            {
                output = Json::Value("PHP");
                return;
            }
            if (input == Currency::CurrencyPKR)
            {
                output = Json::Value("PKR");
                return;
            }
            if (input == Currency::CurrencyPLN)
            {
                output = Json::Value("PLN");
                return;
            }
            if (input == Currency::CurrencyPYG)
            {
                output = Json::Value("PYG");
                return;
            }
            if (input == Currency::CurrencyQAR)
            {
                output = Json::Value("QAR");
                return;
            }
            if (input == Currency::CurrencyRON)
            {
                output = Json::Value("RON");
                return;
            }
            if (input == Currency::CurrencyRSD)
            {
                output = Json::Value("RSD");
                return;
            }
            if (input == Currency::CurrencyRUB)
            {
                output = Json::Value("RUB");
                return;
            }
            if (input == Currency::CurrencyRWF)
            {
                output = Json::Value("RWF");
                return;
            }
            if (input == Currency::CurrencySAR)
            {
                output = Json::Value("SAR");
                return;
            }
            if (input == Currency::CurrencySBD)
            {
                output = Json::Value("SBD");
                return;
            }
            if (input == Currency::CurrencySCR)
            {
                output = Json::Value("SCR");
                return;
            }
            if (input == Currency::CurrencySDG)
            {
                output = Json::Value("SDG");
                return;
            }
            if (input == Currency::CurrencySEK)
            {
                output = Json::Value("SEK");
                return;
            }
            if (input == Currency::CurrencySGD)
            {
                output = Json::Value("SGD");
                return;
            }
            if (input == Currency::CurrencySHP)
            {
                output = Json::Value("SHP");
                return;
            }
            if (input == Currency::CurrencySLL)
            {
                output = Json::Value("SLL");
                return;
            }
            if (input == Currency::CurrencySOS)
            {
                output = Json::Value("SOS");
                return;
            }
            if (input == Currency::CurrencySPL)
            {
                output = Json::Value("SPL");
                return;
            }
            if (input == Currency::CurrencySRD)
            {
                output = Json::Value("SRD");
                return;
            }
            if (input == Currency::CurrencySTD)
            {
                output = Json::Value("STD");
                return;
            }
            if (input == Currency::CurrencySVC)
            {
                output = Json::Value("SVC");
                return;
            }
            if (input == Currency::CurrencySYP)
            {
                output = Json::Value("SYP");
                return;
            }
            if (input == Currency::CurrencySZL)
            {
                output = Json::Value("SZL");
                return;
            }
            if (input == Currency::CurrencyTHB)
            {
                output = Json::Value("THB");
                return;
            }
            if (input == Currency::CurrencyTJS)
            {
                output = Json::Value("TJS");
                return;
            }
            if (input == Currency::CurrencyTMT)
            {
                output = Json::Value("TMT");
                return;
            }
            if (input == Currency::CurrencyTND)
            {
                output = Json::Value("TND");
                return;
            }
            if (input == Currency::CurrencyTOP)
            {
                output = Json::Value("TOP");
                return;
            }
            if (input == Currency::CurrencyTRY)
            {
                output = Json::Value("TRY");
                return;
            }
            if (input == Currency::CurrencyTTD)
            {
                output = Json::Value("TTD");
                return;
            }
            if (input == Currency::CurrencyTVD)
            {
                output = Json::Value("TVD");
                return;
            }
            if (input == Currency::CurrencyTWD)
            {
                output = Json::Value("TWD");
                return;
            }
            if (input == Currency::CurrencyTZS)
            {
                output = Json::Value("TZS");
                return;
            }
            if (input == Currency::CurrencyUAH)
            {
                output = Json::Value("UAH");
                return;
            }
            if (input == Currency::CurrencyUGX)
            {
                output = Json::Value("UGX");
                return;
            }
            if (input == Currency::CurrencyUSD)
            {
                output = Json::Value("USD");
                return;
            }
            if (input == Currency::CurrencyUYU)
            {
                output = Json::Value("UYU");
                return;
            }
            if (input == Currency::CurrencyUZS)
            {
                output = Json::Value("UZS");
                return;
            }
            if (input == Currency::CurrencyVEF)
            {
                output = Json::Value("VEF");
                return;
            }
            if (input == Currency::CurrencyVND)
            {
                output = Json::Value("VND");
                return;
            }
            if (input == Currency::CurrencyVUV)
            {
                output = Json::Value("VUV");
                return;
            }
            if (input == Currency::CurrencyWST)
            {
                output = Json::Value("WST");
                return;
            }
            if (input == Currency::CurrencyXAF)
            {
                output = Json::Value("XAF");
                return;
            }
            if (input == Currency::CurrencyXCD)
            {
                output = Json::Value("XCD");
                return;
            }
            if (input == Currency::CurrencyXDR)
            {
                output = Json::Value("XDR");
                return;
            }
            if (input == Currency::CurrencyXOF)
            {
                output = Json::Value("XOF");
                return;
            }
            if (input == Currency::CurrencyXPF)
            {
                output = Json::Value("XPF");
                return;
            }
            if (input == Currency::CurrencyYER)
            {
                output = Json::Value("YER");
                return;
            }
            if (input == Currency::CurrencyZAR)
            {
                output = Json::Value("ZAR");
                return;
            }
            if (input == Currency::CurrencyZMW)
            {
                output = Json::Value("ZMW");
                return;
            }
            if (input == Currency::CurrencyZWD)
            {
                output = Json::Value("ZWD");
                return;
            }
        }
        inline void FromJsonEnum(const Json::Value& input, Currency& output)
        {
            if (!input.isString())
            {
                return;
            }
            const std::string& inputStr = input.asString();
            if (inputStr == "AED")
            {
                output = Currency::CurrencyAED;
                return;
            }
            if (inputStr == "AFN")
            {
                output = Currency::CurrencyAFN;
                return;
            }
            if (inputStr == "ALL")
            {
                output = Currency::CurrencyALL;
                return;
            }
            if (inputStr == "AMD")
            {
                output = Currency::CurrencyAMD;
                return;
            }
            if (inputStr == "ANG")
            {
                output = Currency::CurrencyANG;
                return;
            }
            if (inputStr == "AOA")
            {
                output = Currency::CurrencyAOA;
                return;
            }
            if (inputStr == "ARS")
            {
                output = Currency::CurrencyARS;
                return;
            }
            if (inputStr == "AUD")
            {
                output = Currency::CurrencyAUD;
                return;
            }
            if (inputStr == "AWG")
            {
                output = Currency::CurrencyAWG;
                return;
            }
            if (inputStr == "AZN")
            {
                output = Currency::CurrencyAZN;
                return;
            }
            if (inputStr == "BAM")
            {
                output = Currency::CurrencyBAM;
                return;
            }
            if (inputStr == "BBD")
            {
                output = Currency::CurrencyBBD;
                return;
            }
            if (inputStr == "BDT")
            {
                output = Currency::CurrencyBDT;
                return;
            }
            if (inputStr == "BGN")
            {
                output = Currency::CurrencyBGN;
                return;
            }
            if (inputStr == "BHD")
            {
                output = Currency::CurrencyBHD;
                return;
            }
            if (inputStr == "BIF")
            {
                output = Currency::CurrencyBIF;
                return;
            }
            if (inputStr == "BMD")
            {
                output = Currency::CurrencyBMD;
                return;
            }
            if (inputStr == "BND")
            {
                output = Currency::CurrencyBND;
                return;
            }
            if (inputStr == "BOB")
            {
                output = Currency::CurrencyBOB;
                return;
            }
            if (inputStr == "BRL")
            {
                output = Currency::CurrencyBRL;
                return;
            }
            if (inputStr == "BSD")
            {
                output = Currency::CurrencyBSD;
                return;
            }
            if (inputStr == "BTN")
            {
                output = Currency::CurrencyBTN;
                return;
            }
            if (inputStr == "BWP")
            {
                output = Currency::CurrencyBWP;
                return;
            }
            if (inputStr == "BYR")
            {
                output = Currency::CurrencyBYR;
                return;
            }
            if (inputStr == "BZD")
            {
                output = Currency::CurrencyBZD;
                return;
            }
            if (inputStr == "CAD")
            {
                output = Currency::CurrencyCAD;
                return;
            }
            if (inputStr == "CDF")
            {
                output = Currency::CurrencyCDF;
                return;
            }
            if (inputStr == "CHF")
            {
                output = Currency::CurrencyCHF;
                return;
            }
            if (inputStr == "CLP")
            {
                output = Currency::CurrencyCLP;
                return;
            }
            if (inputStr == "CNY")
            {
                output = Currency::CurrencyCNY;
                return;
            }
            if (inputStr == "COP")
            {
                output = Currency::CurrencyCOP;
                return;
            }
            if (inputStr == "CRC")
            {
                output = Currency::CurrencyCRC;
                return;
            }
            if (inputStr == "CUC")
            {
                output = Currency::CurrencyCUC;
                return;
            }
            if (inputStr == "CUP")
            {
                output = Currency::CurrencyCUP;
                return;
            }
            if (inputStr == "CVE")
            {
                output = Currency::CurrencyCVE;
                return;
            }
            if (inputStr == "CZK")
            {
                output = Currency::CurrencyCZK;
                return;
            }
            if (inputStr == "DJF")
            {
                output = Currency::CurrencyDJF;
                return;
            }
            if (inputStr == "DKK")
            {
                output = Currency::CurrencyDKK;
                return;
            }
            if (inputStr == "DOP")
            {
                output = Currency::CurrencyDOP;
                return;
            }
            if (inputStr == "DZD")
            {
                output = Currency::CurrencyDZD;
                return;
            }
            if (inputStr == "EGP")
            {
                output = Currency::CurrencyEGP;
                return;
            }
            if (inputStr == "ERN")
            {
                output = Currency::CurrencyERN;
                return;
            }
            if (inputStr == "ETB")
            {
                output = Currency::CurrencyETB;
                return;
            }
            if (inputStr == "EUR")
            {
                output = Currency::CurrencyEUR;
                return;
            }
            if (inputStr == "FJD")
            {
                output = Currency::CurrencyFJD;
                return;
            }
            if (inputStr == "FKP")
            {
                output = Currency::CurrencyFKP;
                return;
            }
            if (inputStr == "GBP")
            {
                output = Currency::CurrencyGBP;
                return;
            }
            if (inputStr == "GEL")
            {
                output = Currency::CurrencyGEL;
                return;
            }
            if (inputStr == "GGP")
            {
                output = Currency::CurrencyGGP;
                return;
            }
            if (inputStr == "GHS")
            {
                output = Currency::CurrencyGHS;
                return;
            }
            if (inputStr == "GIP")
            {
                output = Currency::CurrencyGIP;
                return;
            }
            if (inputStr == "GMD")
            {
                output = Currency::CurrencyGMD;
                return;
            }
            if (inputStr == "GNF")
            {
                output = Currency::CurrencyGNF;
                return;
            }
            if (inputStr == "GTQ")
            {
                output = Currency::CurrencyGTQ;
                return;
            }
            if (inputStr == "GYD")
            {
                output = Currency::CurrencyGYD;
                return;
            }
            if (inputStr == "HKD")
            {
                output = Currency::CurrencyHKD;
                return;
            }
            if (inputStr == "HNL")
            {
                output = Currency::CurrencyHNL;
                return;
            }
            if (inputStr == "HRK")
            {
                output = Currency::CurrencyHRK;
                return;
            }
            if (inputStr == "HTG")
            {
                output = Currency::CurrencyHTG;
                return;
            }
            if (inputStr == "HUF")
            {
                output = Currency::CurrencyHUF;
                return;
            }
            if (inputStr == "IDR")
            {
                output = Currency::CurrencyIDR;
                return;
            }
            if (inputStr == "ILS")
            {
                output = Currency::CurrencyILS;
                return;
            }
            if (inputStr == "IMP")
            {
                output = Currency::CurrencyIMP;
                return;
            }
            if (inputStr == "INR")
            {
                output = Currency::CurrencyINR;
                return;
            }
            if (inputStr == "IQD")
            {
                output = Currency::CurrencyIQD;
                return;
            }
            if (inputStr == "IRR")
            {
                output = Currency::CurrencyIRR;
                return;
            }
            if (inputStr == "ISK")
            {
                output = Currency::CurrencyISK;
                return;
            }
            if (inputStr == "JEP")
            {
                output = Currency::CurrencyJEP;
                return;
            }
            if (inputStr == "JMD")
            {
                output = Currency::CurrencyJMD;
                return;
            }
            if (inputStr == "JOD")
            {
                output = Currency::CurrencyJOD;
                return;
            }
            if (inputStr == "JPY")
            {
                output = Currency::CurrencyJPY;
                return;
            }
            if (inputStr == "KES")
            {
                output = Currency::CurrencyKES;
                return;
            }
            if (inputStr == "KGS")
            {
                output = Currency::CurrencyKGS;
                return;
            }
            if (inputStr == "KHR")
            {
                output = Currency::CurrencyKHR;
                return;
            }
            if (inputStr == "KMF")
            {
                output = Currency::CurrencyKMF;
                return;
            }
            if (inputStr == "KPW")
            {
                output = Currency::CurrencyKPW;
                return;
            }
            if (inputStr == "KRW")
            {
                output = Currency::CurrencyKRW;
                return;
            }
            if (inputStr == "KWD")
            {
                output = Currency::CurrencyKWD;
                return;
            }
            if (inputStr == "KYD")
            {
                output = Currency::CurrencyKYD;
                return;
            }
            if (inputStr == "KZT")
            {
                output = Currency::CurrencyKZT;
                return;
            }
            if (inputStr == "LAK")
            {
                output = Currency::CurrencyLAK;
                return;
            }
            if (inputStr == "LBP")
            {
                output = Currency::CurrencyLBP;
                return;
            }
            if (inputStr == "LKR")
            {
                output = Currency::CurrencyLKR;
                return;
            }
            if (inputStr == "LRD")
            {
                output = Currency::CurrencyLRD;
                return;
            }
            if (inputStr == "LSL")
            {
                output = Currency::CurrencyLSL;
                return;
            }
            if (inputStr == "LYD")
            {
                output = Currency::CurrencyLYD;
                return;
            }
            if (inputStr == "MAD")
            {
                output = Currency::CurrencyMAD;
                return;
            }
            if (inputStr == "MDL")
            {
                output = Currency::CurrencyMDL;
                return;
            }
            if (inputStr == "MGA")
            {
                output = Currency::CurrencyMGA;
                return;
            }
            if (inputStr == "MKD")
            {
                output = Currency::CurrencyMKD;
                return;
            }
            if (inputStr == "MMK")
            {
                output = Currency::CurrencyMMK;
                return;
            }
            if (inputStr == "MNT")
            {
                output = Currency::CurrencyMNT;
                return;
            }
            if (inputStr == "MOP")
            {
                output = Currency::CurrencyMOP;
                return;
            }
            if (inputStr == "MRO")
            {
                output = Currency::CurrencyMRO;
                return;
            }
            if (inputStr == "MUR")
            {
                output = Currency::CurrencyMUR;
                return;
            }
            if (inputStr == "MVR")
            {
                output = Currency::CurrencyMVR;
                return;
            }
            if (inputStr == "MWK")
            {
                output = Currency::CurrencyMWK;
                return;
            }
            if (inputStr == "MXN")
            {
                output = Currency::CurrencyMXN;
                return;
            }
            if (inputStr == "MYR")
            {
                output = Currency::CurrencyMYR;
                return;
            }
            if (inputStr == "MZN")
            {
                output = Currency::CurrencyMZN;
                return;
            }
            if (inputStr == "NAD")
            {
                output = Currency::CurrencyNAD;
                return;
            }
            if (inputStr == "NGN")
            {
                output = Currency::CurrencyNGN;
                return;
            }
            if (inputStr == "NIO")
            {
                output = Currency::CurrencyNIO;
                return;
            }
            if (inputStr == "NOK")
            {
                output = Currency::CurrencyNOK;
                return;
            }
            if (inputStr == "NPR")
            {
                output = Currency::CurrencyNPR;
                return;
            }
            if (inputStr == "NZD")
            {
                output = Currency::CurrencyNZD;
                return;
            }
            if (inputStr == "OMR")
            {
                output = Currency::CurrencyOMR;
                return;
            }
            if (inputStr == "PAB")
            {
                output = Currency::CurrencyPAB;
                return;
            }
            if (inputStr == "PEN")
            {
                output = Currency::CurrencyPEN;
                return;
            }
            if (inputStr == "PGK")
            {
                output = Currency::CurrencyPGK;
                return;
            }
            if (inputStr == "PHP")
            {
                output = Currency::CurrencyPHP;
                return;
            }
            if (inputStr == "PKR")
            {
                output = Currency::CurrencyPKR;
                return;
            }
            if (inputStr == "PLN")
            {
                output = Currency::CurrencyPLN;
                return;
            }
            if (inputStr == "PYG")
            {
                output = Currency::CurrencyPYG;
                return;
            }
            if (inputStr == "QAR")
            {
                output = Currency::CurrencyQAR;
                return;
            }
            if (inputStr == "RON")
            {
                output = Currency::CurrencyRON;
                return;
            }
            if (inputStr == "RSD")
            {
                output = Currency::CurrencyRSD;
                return;
            }
            if (inputStr == "RUB")
            {
                output = Currency::CurrencyRUB;
                return;
            }
            if (inputStr == "RWF")
            {
                output = Currency::CurrencyRWF;
                return;
            }
            if (inputStr == "SAR")
            {
                output = Currency::CurrencySAR;
                return;
            }
            if (inputStr == "SBD")
            {
                output = Currency::CurrencySBD;
                return;
            }
            if (inputStr == "SCR")
            {
                output = Currency::CurrencySCR;
                return;
            }
            if (inputStr == "SDG")
            {
                output = Currency::CurrencySDG;
                return;
            }
            if (inputStr == "SEK")
            {
                output = Currency::CurrencySEK;
                return;
            }
            if (inputStr == "SGD")
            {
                output = Currency::CurrencySGD;
                return;
            }
            if (inputStr == "SHP")
            {
                output = Currency::CurrencySHP;
                return;
            }
            if (inputStr == "SLL")
            {
                output = Currency::CurrencySLL;
                return;
            }
            if (inputStr == "SOS")
            {
                output = Currency::CurrencySOS;
                return;
            }
            if (inputStr == "SPL")
            {
                output = Currency::CurrencySPL;
                return;
            }
            if (inputStr == "SRD")
            {
                output = Currency::CurrencySRD;
                return;
            }
            if (inputStr == "STD")
            {
                output = Currency::CurrencySTD;
                return;
            }
            if (inputStr == "SVC")
            {
                output = Currency::CurrencySVC;
                return;
            }
            if (inputStr == "SYP")
            {
                output = Currency::CurrencySYP;
                return;
            }
            if (inputStr == "SZL")
            {
                output = Currency::CurrencySZL;
                return;
            }
            if (inputStr == "THB")
            {
                output = Currency::CurrencyTHB;
                return;
            }
            if (inputStr == "TJS")
            {
                output = Currency::CurrencyTJS;
                return;
            }
            if (inputStr == "TMT")
            {
                output = Currency::CurrencyTMT;
                return;
            }
            if (inputStr == "TND")
            {
                output = Currency::CurrencyTND;
                return;
            }
            if (inputStr == "TOP")
            {
                output = Currency::CurrencyTOP;
                return;
            }
            if (inputStr == "TRY")
            {
                output = Currency::CurrencyTRY;
                return;
            }
            if (inputStr == "TTD")
            {
                output = Currency::CurrencyTTD;
                return;
            }
            if (inputStr == "TVD")
            {
                output = Currency::CurrencyTVD;
                return;
            }
            if (inputStr == "TWD")
            {
                output = Currency::CurrencyTWD;
                return;
            }
            if (inputStr == "TZS")
            {
                output = Currency::CurrencyTZS;
                return;
            }
            if (inputStr == "UAH")
            {
                output = Currency::CurrencyUAH;
                return;
            }
            if (inputStr == "UGX")
            {
                output = Currency::CurrencyUGX;
                return;
            }
            if (inputStr == "USD")
            {
                output = Currency::CurrencyUSD;
                return;
            }
            if (inputStr == "UYU")
            {
                output = Currency::CurrencyUYU;
                return;
            }
            if (inputStr == "UZS")
            {
                output = Currency::CurrencyUZS;
                return;
            }
            if (inputStr == "VEF")
            {
                output = Currency::CurrencyVEF;
                return;
            }
            if (inputStr == "VND")
            {
                output = Currency::CurrencyVND;
                return;
            }
            if (inputStr == "VUV")
            {
                output = Currency::CurrencyVUV;
                return;
            }
            if (inputStr == "WST")
            {
                output = Currency::CurrencyWST;
                return;
            }
            if (inputStr == "XAF")
            {
                output = Currency::CurrencyXAF;
                return;
            }
            if (inputStr == "XCD")
            {
                output = Currency::CurrencyXCD;
                return;
            }
            if (inputStr == "XDR")
            {
                output = Currency::CurrencyXDR;
                return;
            }
            if (inputStr == "XOF")
            {
                output = Currency::CurrencyXOF;
                return;
            }
            if (inputStr == "XPF")
            {
                output = Currency::CurrencyXPF;
                return;
            }
            if (inputStr == "YER")
            {
                output = Currency::CurrencyYER;
                return;
            }
            if (inputStr == "ZAR")
            {
                output = Currency::CurrencyZAR;
                return;
            }
            if (inputStr == "ZMW")
            {
                output = Currency::CurrencyZMW;
                return;
            }
            if (inputStr == "ZWD")
            {
                output = Currency::CurrencyZWD;
                return;
            }
        }

        enum class EffectType
        {
            EffectTypeAllow,
            EffectTypeDeny
        };

        inline void ToJsonEnum(const EffectType input, Json::Value& output)
        {
            if (input == EffectType::EffectTypeAllow)
            {
                output = Json::Value("Allow");
                return;
            }
            if (input == EffectType::EffectTypeDeny)
            {
                output = Json::Value("Deny");
                return;
            }
        }
        inline void FromJsonEnum(const Json::Value& input, EffectType& output)
        {
            if (!input.isString())
            {
                return;
            }
            const std::string& inputStr = input.asString();
            if (inputStr == "Allow")
            {
                output = EffectType::EffectTypeAllow;
                return;
            }
            if (inputStr == "Deny")
            {
                output = EffectType::EffectTypeDeny;
                return;
            }
        }

        enum class EmailVerificationStatus
        {
            EmailVerificationStatusUnverified,
            EmailVerificationStatusPending,
            EmailVerificationStatusConfirmed
        };

        inline void ToJsonEnum(const EmailVerificationStatus input, Json::Value& output)
        {
            if (input == EmailVerificationStatus::EmailVerificationStatusUnverified)
            {
                output = Json::Value("Unverified");
                return;
            }
            if (input == EmailVerificationStatus::EmailVerificationStatusPending)
            {
                output = Json::Value("Pending");
                return;
            }
            if (input == EmailVerificationStatus::EmailVerificationStatusConfirmed)
            {
                output = Json::Value("Confirmed");
                return;
            }
        }
        inline void FromJsonEnum(const Json::Value& input, EmailVerificationStatus& output)
        {
            if (!input.isString())
            {
                return;
            }
            const std::string& inputStr = input.asString();
            if (inputStr == "Unverified")
            {
                output = EmailVerificationStatus::EmailVerificationStatusUnverified;
                return;
            }
            if (inputStr == "Pending")
            {
                output = EmailVerificationStatus::EmailVerificationStatusPending;
                return;
            }
            if (inputStr == "Confirmed")
            {
                output = EmailVerificationStatus::EmailVerificationStatusConfirmed;
                return;
            }
        }

        enum class GameBuildStatus
        {
            GameBuildStatusAvailable,
            GameBuildStatusValidating,
            GameBuildStatusInvalidBuildPackage,
            GameBuildStatusProcessing,
            GameBuildStatusFailedToProcess
        };

        inline void ToJsonEnum(const GameBuildStatus input, Json::Value& output)
        {
            if (input == GameBuildStatus::GameBuildStatusAvailable)
            {
                output = Json::Value("Available");
                return;
            }
            if (input == GameBuildStatus::GameBuildStatusValidating)
            {
                output = Json::Value("Validating");
                return;
            }
            if (input == GameBuildStatus::GameBuildStatusInvalidBuildPackage)
            {
                output = Json::Value("InvalidBuildPackage");
                return;
            }
            if (input == GameBuildStatus::GameBuildStatusProcessing)
            {
                output = Json::Value("Processing");
                return;
            }
            if (input == GameBuildStatus::GameBuildStatusFailedToProcess)
            {
                output = Json::Value("FailedToProcess");
                return;
            }
        }
        inline void FromJsonEnum(const Json::Value& input, GameBuildStatus& output)
        {
            if (!input.isString())
            {
                return;
            }
            const std::string& inputStr = input.asString();
            if (inputStr == "Available")
            {
                output = GameBuildStatus::GameBuildStatusAvailable;
                return;
            }
            if (inputStr == "Validating")
            {
                output = GameBuildStatus::GameBuildStatusValidating;
                return;
            }
            if (inputStr == "InvalidBuildPackage")
            {
                output = GameBuildStatus::GameBuildStatusInvalidBuildPackage;
                return;
            }
            if (inputStr == "Processing")
            {
                output = GameBuildStatus::GameBuildStatusProcessing;
                return;
            }
            if (inputStr == "FailedToProcess")
            {
                output = GameBuildStatus::GameBuildStatusFailedToProcess;
                return;
            }
        }

        enum class GenericErrorCodes
        {
            GenericErrorCodesSuccess,
            GenericErrorCodesUnkownError,
            GenericErrorCodesInvalidParams,
            GenericErrorCodesAccountNotFound,
            GenericErrorCodesAccountBanned,
            GenericErrorCodesInvalidUsernameOrPassword,
            GenericErrorCodesInvalidTitleId,
            GenericErrorCodesInvalidEmailAddress,
            GenericErrorCodesEmailAddressNotAvailable,
            GenericErrorCodesInvalidUsername,
            GenericErrorCodesInvalidPassword,
            GenericErrorCodesUsernameNotAvailable,
            GenericErrorCodesInvalidSteamTicket,
            GenericErrorCodesAccountAlreadyLinked,
            GenericErrorCodesLinkedAccountAlreadyClaimed,
            GenericErrorCodesInvalidFacebookToken,
            GenericErrorCodesAccountNotLinked,
            GenericErrorCodesFailedByPaymentProvider,
            GenericErrorCodesCouponCodeNotFound,
            GenericErrorCodesInvalidContainerItem,
            GenericErrorCodesContainerNotOwned,
            GenericErrorCodesKeyNotOwned,
            GenericErrorCodesInvalidItemIdInTable,
            GenericErrorCodesInvalidReceipt,
            GenericErrorCodesReceiptAlreadyUsed,
            GenericErrorCodesReceiptCancelled,
            GenericErrorCodesGameNotFound,
            GenericErrorCodesGameModeNotFound,
            GenericErrorCodesInvalidGoogleToken,
            GenericErrorCodesUserIsNotPartOfDeveloper,
            GenericErrorCodesInvalidTitleForDeveloper,
            GenericErrorCodesTitleNameConflicts,
            GenericErrorCodesUserisNotValid,
            GenericErrorCodesValueAlreadyExists,
            GenericErrorCodesBuildNotFound,
            GenericErrorCodesPlayerNotInGame,
            GenericErrorCodesInvalidTicket,
            GenericErrorCodesInvalidDeveloper,
            GenericErrorCodesInvalidOrderInfo,
            GenericErrorCodesRegistrationIncomplete,
            GenericErrorCodesInvalidPlatform,
            GenericErrorCodesUnknownError,
            GenericErrorCodesSteamApplicationNotOwned,
            GenericErrorCodesWrongSteamAccount,
            GenericErrorCodesTitleNotActivated,
            GenericErrorCodesRegistrationSessionNotFound,
            GenericErrorCodesNoSuchMod,
            GenericErrorCodesFileNotFound,
            GenericErrorCodesDuplicateEmail,
            GenericErrorCodesItemNotFound,
            GenericErrorCodesItemNotOwned,
            GenericErrorCodesItemNotRecycleable,
            GenericErrorCodesItemNotAffordable,
            GenericErrorCodesInvalidVirtualCurrency,
            GenericErrorCodesWrongVirtualCurrency,
            GenericErrorCodesWrongPrice,
            GenericErrorCodesNonPositiveValue,
            GenericErrorCodesInvalidRegion,
            GenericErrorCodesRegionAtCapacity,
            GenericErrorCodesServerFailedToStart,
            GenericErrorCodesNameNotAvailable,
            GenericErrorCodesInsufficientFunds,
            GenericErrorCodesInvalidDeviceID,
            GenericErrorCodesInvalidPushNotificationToken,
            GenericErrorCodesNoRemainingUses,
            GenericErrorCodesInvalidPaymentProvider,
            GenericErrorCodesPurchaseInitializationFailure,
            GenericErrorCodesDuplicateUsername,
            GenericErrorCodesInvalidBuyerInfo,
            GenericErrorCodesNoGameModeParamsSet,
            GenericErrorCodesBodyTooLarge,
            GenericErrorCodesReservedWordInBody,
            GenericErrorCodesInvalidTypeInBody,
            GenericErrorCodesInvalidRequest,
            GenericErrorCodesReservedEventName,
            GenericErrorCodesInvalidUserStatistics,
            GenericErrorCodesNotAuthenticated,
            GenericErrorCodesStreamAlreadyExists,
            GenericErrorCodesErrorCreatingStream,
            GenericErrorCodesStreamNotFound,
            GenericErrorCodesInvalidAccount,
            GenericErrorCodesPurchaseDoesNotExist,
            GenericErrorCodesInvalidPurchaseTransactionStatus,
            GenericErrorCodesAPINotEnabledForGameClientAccess,
            GenericErrorCodesNoPushNotificationARNForTitle,
            GenericErrorCodesBuildAlreadyExists,
            GenericErrorCodesBuildPackageDoesNotExist,
            GenericErrorCodesCustomAnalyticsEventsNotEnabledForTitle,
            GenericErrorCodesInvalidSharedGroupId,
            GenericErrorCodesNotAuthorized,
            GenericErrorCodesMissingTitleGoogleProperties,
            GenericErrorCodesInvalidItemProperties,
            GenericErrorCodesInvalidPSNAuthCode,
            GenericErrorCodesInvalidItemId,
            GenericErrorCodesPushNotEnabledForAccount,
            GenericErrorCodesPushServiceError,
            GenericErrorCodesReceiptDoesNotContainInAppItems,
            GenericErrorCodesReceiptContainsMultipleInAppItems,
            GenericErrorCodesInvalidBundleID,
            GenericErrorCodesJavascriptException,
            GenericErrorCodesInvalidSessionTicket,
            GenericErrorCodesUnableToConnectToDatabase,
            GenericErrorCodesInternalServerError,
            GenericErrorCodesInvalidReportDate,
            GenericErrorCodesReportNotAvailable,
            GenericErrorCodesDatabaseThroughputExceeded,
            GenericErrorCodesInvalidGameTicket,
            GenericErrorCodesExpiredGameTicket,
            GenericErrorCodesGameTicketDoesNotMatchLobby,
            GenericErrorCodesLinkedDeviceAlreadyClaimed,
            GenericErrorCodesDeviceAlreadyLinked,
            GenericErrorCodesDeviceNotLinked,
            GenericErrorCodesPartialFailure,
            GenericErrorCodesPublisherNotSet,
            GenericErrorCodesServiceUnavailable,
            GenericErrorCodesVersionNotFound,
            GenericErrorCodesRevisionNotFound,
            GenericErrorCodesInvalidPublisherId,
            GenericErrorCodesDownstreamServiceUnavailable,
            GenericErrorCodesAPINotIncludedInTitleUsageTier,
            GenericErrorCodesDAULimitExceeded,
            GenericErrorCodesAPIRequestLimitExceeded,
            GenericErrorCodesInvalidAPIEndpoint,
            GenericErrorCodesBuildNotAvailable,
            GenericErrorCodesConcurrentEditError,
            GenericErrorCodesContentNotFound,
            GenericErrorCodesCharacterNotFound,
            GenericErrorCodesCloudScriptNotFound,
            GenericErrorCodesContentQuotaExceeded,
            GenericErrorCodesInvalidCharacterStatistics,
            GenericErrorCodesPhotonNotEnabledForTitle,
            GenericErrorCodesPhotonApplicationNotFound,
            GenericErrorCodesPhotonApplicationNotAssociatedWithTitle,
            GenericErrorCodesInvalidEmailOrPassword,
            GenericErrorCodesFacebookAPIError,
            GenericErrorCodesInvalidContentType,
            GenericErrorCodesKeyLengthExceeded,
            GenericErrorCodesDataLengthExceeded,
            GenericErrorCodesTooManyKeys,
            GenericErrorCodesFreeTierCannotHaveVirtualCurrency,
            GenericErrorCodesMissingAmazonSharedKey,
            GenericErrorCodesAmazonValidationError,
            GenericErrorCodesInvalidPSNIssuerId,
            GenericErrorCodesPSNInaccessible,
            GenericErrorCodesExpiredAuthToken,
            GenericErrorCodesFailedToGetEntitlements,
            GenericErrorCodesFailedToConsumeEntitlement,
            GenericErrorCodesTradeAcceptingUserNotAllowed,
            GenericErrorCodesTradeInventoryItemIsAssignedToCharacter,
            GenericErrorCodesTradeInventoryItemIsBundle,
            GenericErrorCodesTradeStatusNotValidForCancelling,
            GenericErrorCodesTradeStatusNotValidForAccepting,
            GenericErrorCodesTradeDoesNotExist,
            GenericErrorCodesTradeCancelled,
            GenericErrorCodesTradeAlreadyFilled,
            GenericErrorCodesTradeWaitForStatusTimeout,
            GenericErrorCodesTradeInventoryItemExpired,
            GenericErrorCodesTradeMissingOfferedAndAcceptedItems,
            GenericErrorCodesTradeAcceptedItemIsBundle,
            GenericErrorCodesTradeAcceptedItemIsStackable,
            GenericErrorCodesTradeInventoryItemInvalidStatus,
            GenericErrorCodesTradeAcceptedCatalogItemInvalid,
            GenericErrorCodesTradeAllowedUsersInvalid,
            GenericErrorCodesTradeInventoryItemDoesNotExist,
            GenericErrorCodesTradeInventoryItemIsConsumed,
            GenericErrorCodesTradeInventoryItemIsStackable,
            GenericErrorCodesTradeAcceptedItemsMismatch,
            GenericErrorCodesInvalidKongregateToken,
            GenericErrorCodesFeatureNotConfiguredForTitle,
            GenericErrorCodesNoMatchingCatalogItemForReceipt,
            GenericErrorCodesInvalidCurrencyCode,
            GenericErrorCodesNoRealMoneyPriceForCatalogItem,
            GenericErrorCodesTradeInventoryItemIsNotTradable,
            GenericErrorCodesTradeAcceptedCatalogItemIsNotTradable,
            GenericErrorCodesUsersAlreadyFriends,
            GenericErrorCodesLinkedIdentifierAlreadyClaimed,
            GenericErrorCodesCustomIdNotLinked,
            GenericErrorCodesTotalDataSizeExceeded,
            GenericErrorCodesDeleteKeyConflict,
            GenericErrorCodesInvalidXboxLiveToken,
            GenericErrorCodesExpiredXboxLiveToken,
            GenericErrorCodesResettableStatisticVersionRequired,
            GenericErrorCodesNotAuthorizedByTitle,
            GenericErrorCodesNoPartnerEnabled,
            GenericErrorCodesInvalidPartnerResponse,
            GenericErrorCodesAPINotEnabledForGameServerAccess,
            GenericErrorCodesStatisticNotFound,
            GenericErrorCodesStatisticNameConflict,
            GenericErrorCodesStatisticVersionClosedForWrites,
            GenericErrorCodesStatisticVersionInvalid,
            GenericErrorCodesAPIClientRequestRateLimitExceeded,
            GenericErrorCodesInvalidJSONContent,
            GenericErrorCodesInvalidDropTable,
            GenericErrorCodesStatisticVersionAlreadyIncrementedForScheduledInterval,
            GenericErrorCodesStatisticCountLimitExceeded,
            GenericErrorCodesStatisticVersionIncrementRateExceeded,
            GenericErrorCodesContainerKeyInvalid,
            GenericErrorCodesCloudScriptExecutionTimeLimitExceeded,
            GenericErrorCodesNoWritePermissionsForEvent,
            GenericErrorCodesCloudScriptFunctionArgumentSizeExceeded,
            GenericErrorCodesCloudScriptAPIRequestCountExceeded,
            GenericErrorCodesCloudScriptAPIRequestError,
            GenericErrorCodesCloudScriptHTTPRequestError,
            GenericErrorCodesInsufficientGuildRole,
            GenericErrorCodesGuildNotFound,
            GenericErrorCodesOverLimit,
            GenericErrorCodesEventNotFound,
            GenericErrorCodesInvalidEventField,
            GenericErrorCodesInvalidEventName,
            GenericErrorCodesCatalogNotConfigured,
            GenericErrorCodesOperationNotSupportedForPlatform,
            GenericErrorCodesSegmentNotFound,
            GenericErrorCodesStoreNotFound,
            GenericErrorCodesInvalidStatisticName,
            GenericErrorCodesTitleNotQualifiedForLimit,
            GenericErrorCodesInvalidServiceLimitLevel,
            GenericErrorCodesServiceLimitLevelInTransition,
            GenericErrorCodesCouponAlreadyRedeemed,
            GenericErrorCodesGameServerBuildSizeLimitExceeded,
            GenericErrorCodesGameServerBuildCountLimitExceeded,
            GenericErrorCodesVirtualCurrencyCountLimitExceeded,
            GenericErrorCodesVirtualCurrencyCodeExists,
            GenericErrorCodesTitleNewsItemCountLimitExceeded,
            GenericErrorCodesInvalidTwitchToken,
            GenericErrorCodesTwitchResponseError,
            GenericErrorCodesProfaneDisplayName,
            GenericErrorCodesUserAlreadyAdded,
            GenericErrorCodesInvalidVirtualCurrencyCode,
            GenericErrorCodesVirtualCurrencyCannotBeDeleted,
            GenericErrorCodesIdentifierAlreadyClaimed,
            GenericErrorCodesIdentifierNotLinked,
            GenericErrorCodesInvalidContinuationToken,
            GenericErrorCodesExpiredContinuationToken,
            GenericErrorCodesInvalidSegment,
            GenericErrorCodesInvalidSessionId,
            GenericErrorCodesSessionLogNotFound,
            GenericErrorCodesInvalidSearchTerm,
            GenericErrorCodesTwoFactorAuthenticationTokenRequired,
            GenericErrorCodesGameServerHostCountLimitExceeded,
            GenericErrorCodesPlayerTagCountLimitExceeded,
            GenericErrorCodesRequestAlreadyRunning,
            GenericErrorCodesActionGroupNotFound,
            GenericErrorCodesMaximumSegmentBulkActionJobsRunning,
            GenericErrorCodesNoActionsOnPlayersInSegmentJob,
            GenericErrorCodesDuplicateStatisticName,
            GenericErrorCodesScheduledTaskNameConflict,
            GenericErrorCodesScheduledTaskCreateConflict,
            GenericErrorCodesInvalidScheduledTaskName,
            GenericErrorCodesInvalidTaskSchedule,
            GenericErrorCodesSteamNotEnabledForTitle,
            GenericErrorCodesLimitNotAnUpgradeOption,
            GenericErrorCodesNoSecretKeyEnabledForCloudScript,
            GenericErrorCodesTaskNotFound,
            GenericErrorCodesTaskInstanceNotFound,
            GenericErrorCodesInvalidIdentityProviderId,
            GenericErrorCodesMisconfiguredIdentityProvider,
            GenericErrorCodesInvalidScheduledTaskType,
            GenericErrorCodesBillingInformationRequired,
            GenericErrorCodesLimitedEditionItemUnavailable,
            GenericErrorCodesInvalidAdPlacementAndReward,
            GenericErrorCodesAllAdPlacementViewsAlreadyConsumed,
            GenericErrorCodesGoogleOAuthNotConfiguredForTitle,
            GenericErrorCodesGoogleOAuthError,
            GenericErrorCodesUserNotFriend,
            GenericErrorCodesInvalidSignature,
            GenericErrorCodesInvalidPublicKey,
            GenericErrorCodesGoogleOAuthNoIdTokenIncludedInResponse,
            GenericErrorCodesStatisticUpdateInProgress,
            GenericErrorCodesLeaderboardVersionNotAvailable,
            GenericErrorCodesStatisticAlreadyHasPrizeTable,
            GenericErrorCodesPrizeTableHasOverlappingRanks,
            GenericErrorCodesPrizeTableHasMissingRanks,
            GenericErrorCodesPrizeTableRankStartsAtZero,
            GenericErrorCodesInvalidStatistic,
            GenericErrorCodesExpressionParseFailure,
            GenericErrorCodesExpressionInvokeFailure,
            GenericErrorCodesExpressionTooLong,
            GenericErrorCodesDataUpdateRateExceeded,
            GenericErrorCodesRestrictedEmailDomain,
            GenericErrorCodesEncryptionKeyDisabled,
            GenericErrorCodesEncryptionKeyMissing,
            GenericErrorCodesEncryptionKeyBroken,
            GenericErrorCodesNoSharedSecretKeyConfigured,
            GenericErrorCodesSecretKeyNotFound,
            GenericErrorCodesPlayerSecretAlreadyConfigured,
            GenericErrorCodesAPIRequestsDisabledForTitle,
            GenericErrorCodesInvalidSharedSecretKey,
            GenericErrorCodesPrizeTableHasNoRanks,
            GenericErrorCodesProfileDoesNotExist,
            GenericErrorCodesContentS3OriginBucketNotConfigured,
            GenericErrorCodesInvalidEnvironmentForReceipt,
            GenericErrorCodesEncryptedRequestNotAllowed,
            GenericErrorCodesSignedRequestNotAllowed,
            GenericErrorCodesRequestViewConstraintParamsNotAllowed,
            GenericErrorCodesBadPartnerConfiguration,
            GenericErrorCodesXboxBPCertificateFailure,
            GenericErrorCodesXboxXASSExchangeFailure,
            GenericErrorCodesInvalidEntityId,
            GenericErrorCodesStatisticValueAggregationOverflow,
            GenericErrorCodesEmailMessageFromAddressIsMissing,
            GenericErrorCodesEmailMessageToAddressIsMissing,
            GenericErrorCodesSmtpServerAuthenticationError,
            GenericErrorCodesSmtpServerLimitExceeded,
            GenericErrorCodesSmtpServerInsufficientStorage,
            GenericErrorCodesSmtpServerCommunicationError,
            GenericErrorCodesSmtpServerGeneralFailure,
            GenericErrorCodesEmailClientTimeout,
            GenericErrorCodesEmailClientCanceledTask,
            GenericErrorCodesEmailTemplateMissing,
            GenericErrorCodesInvalidHostForTitleId,
            GenericErrorCodesEmailConfirmationTokenDoesNotExist,
            GenericErrorCodesEmailConfirmationTokenExpired,
            GenericErrorCodesAccountDeleted,
            GenericErrorCodesPlayerSecretNotConfigured,
            GenericErrorCodesInvalidSignatureTime,
            GenericErrorCodesNoContactEmailAddressFound,
            GenericErrorCodesInvalidAuthToken,
            GenericErrorCodesAuthTokenDoesNotExist,
            GenericErrorCodesAuthTokenExpired,
            GenericErrorCodesAuthTokenAlreadyUsedToResetPassword,
            GenericErrorCodesMembershipNameTooLong,
            GenericErrorCodesMembershipNotFound,
            GenericErrorCodesGoogleServiceAccountInvalid,
            GenericErrorCodesGoogleServiceAccountParseFailure,
            GenericErrorCodesEntityTokenMissing,
            GenericErrorCodesEntityTokenInvalid,
            GenericErrorCodesEntityTokenExpired,
            GenericErrorCodesEntityTokenRevoked,
            GenericErrorCodesInvalidProductForSubscription,
            GenericErrorCodesXboxInaccessible,
            GenericErrorCodesSubscriptionAlreadyTaken,
            GenericErrorCodesSmtpAddonNotEnabled,
            GenericErrorCodesAPIConcurrentRequestLimitExceeded,
            GenericErrorCodesXboxRejectedXSTSExchangeRequest,
            GenericErrorCodesVariableNotDefined,
            GenericErrorCodesTemplateVersionNotDefined,
            GenericErrorCodesFileTooLarge,
            GenericErrorCodesTitleDeleted,
            GenericErrorCodesTitleContainsUserAccounts,
            GenericErrorCodesTitleDeletionPlayerCleanupFailure,
            GenericErrorCodesEntityFileOperationPending,
            GenericErrorCodesNoEntityFileOperationPending,
            GenericErrorCodesEntityProfileVersionMismatch,
            GenericErrorCodesTemplateVersionTooOld,
            GenericErrorCodesMembershipDefinitionInUse,
            GenericErrorCodesPaymentPageNotConfigured,
            GenericErrorCodesFailedLoginAttemptRateLimitExceeded,
            GenericErrorCodesEntityBlockedByGroup,
            GenericErrorCodesRoleDoesNotExist,
            GenericErrorCodesEntityIsAlreadyMember,
            GenericErrorCodesDuplicateRoleId,
            GenericErrorCodesGroupInvitationNotFound,
            GenericErrorCodesGroupApplicationNotFound,
            GenericErrorCodesOutstandingInvitationAcceptedInstead,
            GenericErrorCodesOutstandingApplicationAcceptedInstead,
            GenericErrorCodesRoleIsGroupDefaultMember,
            GenericErrorCodesRoleIsGroupAdmin,
            GenericErrorCodesRoleNameNotAvailable,
            GenericErrorCodesGroupNameNotAvailable,
            GenericErrorCodesEmailReportAlreadySent,
            GenericErrorCodesEmailReportRecipientBlacklisted,
            GenericErrorCodesEventNamespaceNotAllowed,
            GenericErrorCodesEventEntityNotAllowed,
            GenericErrorCodesInvalidEntityType,
            GenericErrorCodesNullTokenResultFromAad,
            GenericErrorCodesInvalidTokenResultFromAad,
            GenericErrorCodesNoValidCertificateForAad,
            GenericErrorCodesInvalidCertificateForAad,
            GenericErrorCodesDuplicateDropTableId,
            GenericErrorCodesMultiplayerServerError,
            GenericErrorCodesMultiplayerServerTooManyRequests,
            GenericErrorCodesMultiplayerServerNoContent,
            GenericErrorCodesMultiplayerServerBadRequest,
            GenericErrorCodesMultiplayerServerUnauthorized,
            GenericErrorCodesMultiplayerServerForbidden,
            GenericErrorCodesMultiplayerServerNotFound,
            GenericErrorCodesMultiplayerServerConflict,
            GenericErrorCodesMultiplayerServerInternalServerError,
            GenericErrorCodesMultiplayerServerUnavailable,
            GenericErrorCodesExplicitContentDetected,
            GenericErrorCodesPIIContentDetected,
            GenericErrorCodesInvalidScheduledTaskParameter,
            GenericErrorCodesPerEntityEventRateLimitExceeded,
            GenericErrorCodesTitleDefaultLanguageNotSet,
            GenericErrorCodesEmailTemplateMissingDefaultVersion,
            GenericErrorCodesFacebookInstantGamesIdNotLinked,
            GenericErrorCodesInvalidFacebookInstantGamesSignature,
            GenericErrorCodesFacebookInstantGamesAuthNotConfiguredForTitle,
            GenericErrorCodesEntityProfileConstraintValidationFailed,
            GenericErrorCodesTelemetryIngestionKeyPending,
            GenericErrorCodesTelemetryIngestionKeyNotFound,
            GenericErrorCodesStatisticChildNameInvalid,
            GenericErrorCodesDataIntegrityError,
            GenericErrorCodesVirtualCurrencyCannotBeSetToOlderVersion,
            GenericErrorCodesVirtualCurrencyMustBeWithinIntegerRange,
            GenericErrorCodesEmailTemplateInvalidSyntax,
            GenericErrorCodesEmailTemplateMissingCallback,
            GenericErrorCodesPushNotificationTemplateInvalidPayload,
            GenericErrorCodesInvalidLocalizedPushNotificationLanguage,
            GenericErrorCodesMissingLocalizedPushNotificationMessage,
            GenericErrorCodesPushNotificationTemplateMissingPlatformPayload,
            GenericErrorCodesPushNotificationTemplatePayloadContainsInvalidJson,
            GenericErrorCodesPushNotificationTemplateContainsInvalidIosPayload,
            GenericErrorCodesPushNotificationTemplateContainsInvalidAndroidPayload,
            GenericErrorCodesPushNotificationTemplateIosPayloadMissingNotificationBody,
            GenericErrorCodesPushNotificationTemplateAndroidPayloadMissingNotificationBody,
            GenericErrorCodesPushNotificationTemplateNotFound,
            GenericErrorCodesPushNotificationTemplateMissingDefaultVersion,
            GenericErrorCodesPushNotificationTemplateInvalidSyntax,
            GenericErrorCodesPushNotificationTemplateNoCustomPayloadForV1,
            GenericErrorCodesNoLeaderboardForStatistic,
            GenericErrorCodesTitleNewsMissingDefaultLanguage,
            GenericErrorCodesTitleNewsNotFound,
            GenericErrorCodesTitleNewsDuplicateLanguage,
            GenericErrorCodesTitleNewsMissingTitleOrBody,
            GenericErrorCodesTitleNewsInvalidLanguage,
            GenericErrorCodesEmailRecipientBlacklisted,
            GenericErrorCodesInvalidGameCenterAuthRequest,
            GenericErrorCodesGameCenterAuthenticationFailed,
            GenericErrorCodesCannotEnablePartiesForTitle,
            GenericErrorCodesPartyError,
            GenericErrorCodesPartyRequests,
            GenericErrorCodesPartyNoContent,
            GenericErrorCodesPartyBadRequest,
            GenericErrorCodesPartyUnauthorized,
            GenericErrorCodesPartyForbidden,
            GenericErrorCodesPartyNotFound,
            GenericErrorCodesPartyConflict,
            GenericErrorCodesPartyInternalServerError,
            GenericErrorCodesPartyUnavailable,
            GenericErrorCodesPartyTooManyRequests,
            GenericErrorCodesPushNotificationTemplateMissingName,
            GenericErrorCodesCannotEnableMultiplayerServersForTitle,
            GenericErrorCodesWriteAttemptedDuringExport,
            GenericErrorCodesMultiplayerServerTitleQuotaCoresExceeded,
            GenericErrorCodesAutomationRuleNotFound,
            GenericErrorCodesEntityAPIKeyLimitExceeded,
            GenericErrorCodesEntityAPIKeyNotFound,
            GenericErrorCodesEntityAPIKeyOrSecretInvalid,
            GenericErrorCodesEconomyServiceUnavailable,
            GenericErrorCodesEconomyServiceInternalError,
            GenericErrorCodesQueryRateLimitExceeded,
            GenericErrorCodesEntityAPIKeyCreationDisabledForEntity,
            GenericErrorCodesForbiddenByEntityPolicy,
            GenericErrorCodesUpdateInventoryRateLimitExceeded,
            GenericErrorCodesStudioCreationRateLimited,
            GenericErrorCodesStudioCreationInProgress,
            GenericErrorCodesDuplicateStudioName,
            GenericErrorCodesStudioNotFound,
            GenericErrorCodesStudioDeleted,
            GenericErrorCodesStudioDeactivated,
            GenericErrorCodesStudioActivated,
            GenericErrorCodesTitleCreationRateLimited,
            GenericErrorCodesTitleCreationInProgress,
            GenericErrorCodesDuplicateTitleName,
            GenericErrorCodesTitleActivationRateLimited,
            GenericErrorCodesTitleActivationInProgress,
            GenericErrorCodesTitleDeactivated,
            GenericErrorCodesTitleActivated,
            GenericErrorCodesCloudScriptAzureFunctionsExecutionTimeLimitExceeded,
            GenericErrorCodesCloudScriptAzureFunctionsArgumentSizeExceeded,
            GenericErrorCodesCloudScriptAzureFunctionsReturnSizeExceeded,
            GenericErrorCodesCloudScriptAzureFunctionsHTTPRequestError,
            GenericErrorCodesVirtualCurrencyBetaGetError,
            GenericErrorCodesVirtualCurrencyBetaCreateError,
            GenericErrorCodesVirtualCurrencyBetaInitialDepositSaveError,
            GenericErrorCodesVirtualCurrencyBetaSaveError,
            GenericErrorCodesVirtualCurrencyBetaDeleteError,
            GenericErrorCodesVirtualCurrencyBetaRestoreError,
            GenericErrorCodesVirtualCurrencyBetaSaveConflict,
            GenericErrorCodesVirtualCurrencyBetaUpdateError,
            GenericErrorCodesInsightsManagementDatabaseNotFound,
            GenericErrorCodesInsightsManagementOperationNotFound,
            GenericErrorCodesInsightsManagementErrorPendingOperationExists,
            GenericErrorCodesInsightsManagementSetPerformanceLevelInvalidParameter,
            GenericErrorCodesInsightsManagementSetStorageRetentionInvalidParameter,
            GenericErrorCodesInsightsManagementGetStorageUsageInvalidParameter,
            GenericErrorCodesInsightsManagementGetOperationStatusInvalidParameter,
            GenericErrorCodesDuplicatePurchaseTransactionId,
            GenericErrorCodesEvaluationModePlayerCountExceeded,
            GenericErrorCodesGetPlayersInSegmentRateLimitExceeded,
            GenericErrorCodesCloudScriptFunctionNameSizeExceeded,
            GenericErrorCodesInsightsManagementTitleInEvaluationMode,
            GenericErrorCodesCloudScriptAzureFunctionsQueueRequestError,
            GenericErrorCodesEvaluationModeTitleCountExceeded,
            GenericErrorCodesInsightsManagementTitleNotInFlight,
            GenericErrorCodesLimitNotFound,
            GenericErrorCodesLimitNotAvailableViaAPI,
            GenericErrorCodesInsightsManagementSetStorageRetentionBelowMinimum,
            GenericErrorCodesInsightsManagementSetStorageRetentionAboveMaximum,
            GenericErrorCodesAppleNotEnabledForTitle,
            GenericErrorCodesInsightsManagementNewActiveEventExportLimitInvalid,
            GenericErrorCodesInsightsManagementSetPerformanceRateLimited,
            GenericErrorCodesPartyRequestsThrottledFromRateLimiter,
            GenericErrorCodesXboxServiceTooManyRequests,
            GenericErrorCodesNintendoSwitchNotEnabledForTitle,
            GenericErrorCodesRequestMultiplayerServersThrottledFromRateLimiter,
            GenericErrorCodesTitleDataOverrideNotFound,
            GenericErrorCodesDuplicateKeys,
            GenericErrorCodesWasNotCreatedWithCloudRoot,
            GenericErrorCodesLegacyMultiplayerServersDeprecated,
            GenericErrorCodesMatchmakingEntityInvalid,
            GenericErrorCodesMatchmakingPlayerAttributesInvalid,
            GenericErrorCodesMatchmakingQueueNotFound,
            GenericErrorCodesMatchmakingMatchNotFound,
            GenericErrorCodesMatchmakingTicketNotFound,
            GenericErrorCodesMatchmakingAlreadyJoinedTicket,
            GenericErrorCodesMatchmakingTicketAlreadyCompleted,
            GenericErrorCodesMatchmakingQueueConfigInvalid,
            GenericErrorCodesMatchmakingMemberProfileInvalid,
            GenericErrorCodesNintendoSwitchDeviceIdNotLinked,
            GenericErrorCodesMatchmakingNotEnabled,
            GenericErrorCodesMatchmakingPlayerAttributesTooLarge,
            GenericErrorCodesMatchmakingNumberOfPlayersInTicketTooLarge,
            GenericErrorCodesMatchmakingAttributeInvalid,
            GenericErrorCodesMatchmakingPlayerHasNotJoinedTicket,
            GenericErrorCodesMatchmakingRateLimitExceeded,
            GenericErrorCodesMatchmakingTicketMembershipLimitExceeded,
            GenericErrorCodesMatchmakingUnauthorized,
            GenericErrorCodesMatchmakingQueueLimitExceeded,
            GenericErrorCodesMatchmakingRequestTypeMismatch,
            GenericErrorCodesMatchmakingBadRequest,
            GenericErrorCodesTitleConfigNotFound,
            GenericErrorCodesTitleConfigUpdateConflict,
            GenericErrorCodesTitleConfigSerializationError,
            GenericErrorCodesCatalogEntityInvalid,
            GenericErrorCodesCatalogTitleIdMissing,
            GenericErrorCodesCatalogPlayerIdMissing,
            GenericErrorCodesCatalogClientIdentityInvalid,
            GenericErrorCodesCatalogOneOrMoreFilesInvalid,
            GenericErrorCodesCatalogItemMetadataInvalid,
            GenericErrorCodesCatalogItemIdInvalid,
            GenericErrorCodesCatalogSearchParameterInvalid,
            GenericErrorCodesCatalogFeatureDisabled,
            GenericErrorCodesCatalogConfigInvalid,
            GenericErrorCodesCatalogUnauthorized,
            GenericErrorCodesCatalogItemTypeInvalid,
            GenericErrorCodesCatalogBadRequest,
            GenericErrorCodesCatalogTooManyRequests,
            GenericErrorCodesExportInvalidStatusUpdate,
            GenericErrorCodesExportInvalidPrefix,
            GenericErrorCodesExportBlobContainerDoesNotExist,
            GenericErrorCodesExportNotFound,
            GenericErrorCodesExportCouldNotUpdate,
            GenericErrorCodesExportInvalidStorageType,
            GenericErrorCodesExportAmazonBucketDoesNotExist,
            GenericErrorCodesExportInvalidBlobStorage,
            GenericErrorCodesExportKustoException,
            GenericErrorCodesExportKustoConnectionFailed,
            GenericErrorCodesExportUnknownError,
            GenericErrorCodesExportCantEditPendingExport,
            GenericErrorCodesExportLimitExports,
            GenericErrorCodesExportLimitEvents,
            GenericErrorCodesExportInvalidPartitionStatusModification,
            GenericErrorCodesExportCouldNotCreate,
            GenericErrorCodesExportNoBackingDatabaseFound,
            GenericErrorCodesExportCouldNotDelete,
            GenericErrorCodesExportCannotDetermineEventQuery,
            GenericErrorCodesExportInvalidQuerySchemaModification,
            GenericErrorCodesExportQuerySchemaMissingRequiredColumns,
            GenericErrorCodesExportCannotParseQuery,
            GenericErrorCodesExportControlCommandsNotAllowed,
            GenericErrorCodesExportQueryMissingTableReference,
            GenericErrorCodesTitleNotEnabledForParty,
            GenericErrorCodesPartyVersionNotFound,
            GenericErrorCodesMultiplayerServerBuildReferencedByMatchmakingQueue,
            GenericErrorCodesExperimentationExperimentStopped,
            GenericErrorCodesExperimentationExperimentRunning,
            GenericErrorCodesExperimentationExperimentNotFound,
            GenericErrorCodesExperimentationExperimentNeverStarted,
            GenericErrorCodesExperimentationExperimentDeleted,
            GenericErrorCodesExperimentationClientTimeout,
            GenericErrorCodesExperimentationInvalidVariantConfiguration,
            GenericErrorCodesExperimentationInvalidVariableConfiguration,
            GenericErrorCodesExperimentInvalidId,
            GenericErrorCodesExperimentationNoScorecard,
            GenericErrorCodesExperimentationTreatmentAssignmentFailed,
            GenericErrorCodesExperimentationTreatmentAssignmentDisabled,
            GenericErrorCodesExperimentationInvalidDuration,
            GenericErrorCodesExperimentationMaxExperimentsReached,
            GenericErrorCodesExperimentationExperimentSchedulingInProgress,
            GenericErrorCodesMaxActionDepthExceeded,
            GenericErrorCodesTitleNotOnUpdatedPricingPlan,
            GenericErrorCodesSnapshotNotFound
        };

        inline void ToJsonEnum(const GenericErrorCodes input, Json::Value& output)
        {
            if (input == GenericErrorCodes::GenericErrorCodesSuccess)
            {
                output = Json::Value("Success");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesUnkownError)
            {
                output = Json::Value("UnkownError");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesInvalidParams)
            {
                output = Json::Value("InvalidParams");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesAccountNotFound)
            {
                output = Json::Value("AccountNotFound");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesAccountBanned)
            {
                output = Json::Value("AccountBanned");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesInvalidUsernameOrPassword)
            {
                output = Json::Value("InvalidUsernameOrPassword");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesInvalidTitleId)
            {
                output = Json::Value("InvalidTitleId");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesInvalidEmailAddress)
            {
                output = Json::Value("InvalidEmailAddress");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesEmailAddressNotAvailable)
            {
                output = Json::Value("EmailAddressNotAvailable");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesInvalidUsername)
            {
                output = Json::Value("InvalidUsername");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesInvalidPassword)
            {
                output = Json::Value("InvalidPassword");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesUsernameNotAvailable)
            {
                output = Json::Value("UsernameNotAvailable");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesInvalidSteamTicket)
            {
                output = Json::Value("InvalidSteamTicket");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesAccountAlreadyLinked)
            {
                output = Json::Value("AccountAlreadyLinked");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesLinkedAccountAlreadyClaimed)
            {
                output = Json::Value("LinkedAccountAlreadyClaimed");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesInvalidFacebookToken)
            {
                output = Json::Value("InvalidFacebookToken");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesAccountNotLinked)
            {
                output = Json::Value("AccountNotLinked");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesFailedByPaymentProvider)
            {
                output = Json::Value("FailedByPaymentProvider");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesCouponCodeNotFound)
            {
                output = Json::Value("CouponCodeNotFound");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesInvalidContainerItem)
            {
                output = Json::Value("InvalidContainerItem");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesContainerNotOwned)
            {
                output = Json::Value("ContainerNotOwned");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesKeyNotOwned)
            {
                output = Json::Value("KeyNotOwned");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesInvalidItemIdInTable)
            {
                output = Json::Value("InvalidItemIdInTable");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesInvalidReceipt)
            {
                output = Json::Value("InvalidReceipt");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesReceiptAlreadyUsed)
            {
                output = Json::Value("ReceiptAlreadyUsed");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesReceiptCancelled)
            {
                output = Json::Value("ReceiptCancelled");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesGameNotFound)
            {
                output = Json::Value("GameNotFound");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesGameModeNotFound)
            {
                output = Json::Value("GameModeNotFound");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesInvalidGoogleToken)
            {
                output = Json::Value("InvalidGoogleToken");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesUserIsNotPartOfDeveloper)
            {
                output = Json::Value("UserIsNotPartOfDeveloper");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesInvalidTitleForDeveloper)
            {
                output = Json::Value("InvalidTitleForDeveloper");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesTitleNameConflicts)
            {
                output = Json::Value("TitleNameConflicts");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesUserisNotValid)
            {
                output = Json::Value("UserisNotValid");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesValueAlreadyExists)
            {
                output = Json::Value("ValueAlreadyExists");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesBuildNotFound)
            {
                output = Json::Value("BuildNotFound");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesPlayerNotInGame)
            {
                output = Json::Value("PlayerNotInGame");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesInvalidTicket)
            {
                output = Json::Value("InvalidTicket");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesInvalidDeveloper)
            {
                output = Json::Value("InvalidDeveloper");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesInvalidOrderInfo)
            {
                output = Json::Value("InvalidOrderInfo");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesRegistrationIncomplete)
            {
                output = Json::Value("RegistrationIncomplete");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesInvalidPlatform)
            {
                output = Json::Value("InvalidPlatform");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesUnknownError)
            {
                output = Json::Value("UnknownError");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesSteamApplicationNotOwned)
            {
                output = Json::Value("SteamApplicationNotOwned");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesWrongSteamAccount)
            {
                output = Json::Value("WrongSteamAccount");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesTitleNotActivated)
            {
                output = Json::Value("TitleNotActivated");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesRegistrationSessionNotFound)
            {
                output = Json::Value("RegistrationSessionNotFound");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesNoSuchMod)
            {
                output = Json::Value("NoSuchMod");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesFileNotFound)
            {
                output = Json::Value("FileNotFound");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesDuplicateEmail)
            {
                output = Json::Value("DuplicateEmail");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesItemNotFound)
            {
                output = Json::Value("ItemNotFound");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesItemNotOwned)
            {
                output = Json::Value("ItemNotOwned");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesItemNotRecycleable)
            {
                output = Json::Value("ItemNotRecycleable");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesItemNotAffordable)
            {
                output = Json::Value("ItemNotAffordable");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesInvalidVirtualCurrency)
            {
                output = Json::Value("InvalidVirtualCurrency");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesWrongVirtualCurrency)
            {
                output = Json::Value("WrongVirtualCurrency");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesWrongPrice)
            {
                output = Json::Value("WrongPrice");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesNonPositiveValue)
            {
                output = Json::Value("NonPositiveValue");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesInvalidRegion)
            {
                output = Json::Value("InvalidRegion");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesRegionAtCapacity)
            {
                output = Json::Value("RegionAtCapacity");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesServerFailedToStart)
            {
                output = Json::Value("ServerFailedToStart");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesNameNotAvailable)
            {
                output = Json::Value("NameNotAvailable");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesInsufficientFunds)
            {
                output = Json::Value("InsufficientFunds");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesInvalidDeviceID)
            {
                output = Json::Value("InvalidDeviceID");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesInvalidPushNotificationToken)
            {
                output = Json::Value("InvalidPushNotificationToken");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesNoRemainingUses)
            {
                output = Json::Value("NoRemainingUses");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesInvalidPaymentProvider)
            {
                output = Json::Value("InvalidPaymentProvider");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesPurchaseInitializationFailure)
            {
                output = Json::Value("PurchaseInitializationFailure");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesDuplicateUsername)
            {
                output = Json::Value("DuplicateUsername");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesInvalidBuyerInfo)
            {
                output = Json::Value("InvalidBuyerInfo");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesNoGameModeParamsSet)
            {
                output = Json::Value("NoGameModeParamsSet");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesBodyTooLarge)
            {
                output = Json::Value("BodyTooLarge");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesReservedWordInBody)
            {
                output = Json::Value("ReservedWordInBody");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesInvalidTypeInBody)
            {
                output = Json::Value("InvalidTypeInBody");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesInvalidRequest)
            {
                output = Json::Value("InvalidRequest");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesReservedEventName)
            {
                output = Json::Value("ReservedEventName");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesInvalidUserStatistics)
            {
                output = Json::Value("InvalidUserStatistics");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesNotAuthenticated)
            {
                output = Json::Value("NotAuthenticated");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesStreamAlreadyExists)
            {
                output = Json::Value("StreamAlreadyExists");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesErrorCreatingStream)
            {
                output = Json::Value("ErrorCreatingStream");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesStreamNotFound)
            {
                output = Json::Value("StreamNotFound");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesInvalidAccount)
            {
                output = Json::Value("InvalidAccount");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesPurchaseDoesNotExist)
            {
                output = Json::Value("PurchaseDoesNotExist");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesInvalidPurchaseTransactionStatus)
            {
                output = Json::Value("InvalidPurchaseTransactionStatus");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesAPINotEnabledForGameClientAccess)
            {
                output = Json::Value("APINotEnabledForGameClientAccess");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesNoPushNotificationARNForTitle)
            {
                output = Json::Value("NoPushNotificationARNForTitle");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesBuildAlreadyExists)
            {
                output = Json::Value("BuildAlreadyExists");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesBuildPackageDoesNotExist)
            {
                output = Json::Value("BuildPackageDoesNotExist");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesCustomAnalyticsEventsNotEnabledForTitle)
            {
                output = Json::Value("CustomAnalyticsEventsNotEnabledForTitle");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesInvalidSharedGroupId)
            {
                output = Json::Value("InvalidSharedGroupId");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesNotAuthorized)
            {
                output = Json::Value("NotAuthorized");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesMissingTitleGoogleProperties)
            {
                output = Json::Value("MissingTitleGoogleProperties");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesInvalidItemProperties)
            {
                output = Json::Value("InvalidItemProperties");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesInvalidPSNAuthCode)
            {
                output = Json::Value("InvalidPSNAuthCode");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesInvalidItemId)
            {
                output = Json::Value("InvalidItemId");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesPushNotEnabledForAccount)
            {
                output = Json::Value("PushNotEnabledForAccount");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesPushServiceError)
            {
                output = Json::Value("PushServiceError");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesReceiptDoesNotContainInAppItems)
            {
                output = Json::Value("ReceiptDoesNotContainInAppItems");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesReceiptContainsMultipleInAppItems)
            {
                output = Json::Value("ReceiptContainsMultipleInAppItems");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesInvalidBundleID)
            {
                output = Json::Value("InvalidBundleID");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesJavascriptException)
            {
                output = Json::Value("JavascriptException");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesInvalidSessionTicket)
            {
                output = Json::Value("InvalidSessionTicket");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesUnableToConnectToDatabase)
            {
                output = Json::Value("UnableToConnectToDatabase");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesInternalServerError)
            {
                output = Json::Value("InternalServerError");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesInvalidReportDate)
            {
                output = Json::Value("InvalidReportDate");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesReportNotAvailable)
            {
                output = Json::Value("ReportNotAvailable");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesDatabaseThroughputExceeded)
            {
                output = Json::Value("DatabaseThroughputExceeded");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesInvalidGameTicket)
            {
                output = Json::Value("InvalidGameTicket");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesExpiredGameTicket)
            {
                output = Json::Value("ExpiredGameTicket");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesGameTicketDoesNotMatchLobby)
            {
                output = Json::Value("GameTicketDoesNotMatchLobby");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesLinkedDeviceAlreadyClaimed)
            {
                output = Json::Value("LinkedDeviceAlreadyClaimed");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesDeviceAlreadyLinked)
            {
                output = Json::Value("DeviceAlreadyLinked");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesDeviceNotLinked)
            {
                output = Json::Value("DeviceNotLinked");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesPartialFailure)
            {
                output = Json::Value("PartialFailure");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesPublisherNotSet)
            {
                output = Json::Value("PublisherNotSet");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesServiceUnavailable)
            {
                output = Json::Value("ServiceUnavailable");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesVersionNotFound)
            {
                output = Json::Value("VersionNotFound");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesRevisionNotFound)
            {
                output = Json::Value("RevisionNotFound");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesInvalidPublisherId)
            {
                output = Json::Value("InvalidPublisherId");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesDownstreamServiceUnavailable)
            {
                output = Json::Value("DownstreamServiceUnavailable");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesAPINotIncludedInTitleUsageTier)
            {
                output = Json::Value("APINotIncludedInTitleUsageTier");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesDAULimitExceeded)
            {
                output = Json::Value("DAULimitExceeded");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesAPIRequestLimitExceeded)
            {
                output = Json::Value("APIRequestLimitExceeded");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesInvalidAPIEndpoint)
            {
                output = Json::Value("InvalidAPIEndpoint");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesBuildNotAvailable)
            {
                output = Json::Value("BuildNotAvailable");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesConcurrentEditError)
            {
                output = Json::Value("ConcurrentEditError");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesContentNotFound)
            {
                output = Json::Value("ContentNotFound");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesCharacterNotFound)
            {
                output = Json::Value("CharacterNotFound");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesCloudScriptNotFound)
            {
                output = Json::Value("CloudScriptNotFound");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesContentQuotaExceeded)
            {
                output = Json::Value("ContentQuotaExceeded");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesInvalidCharacterStatistics)
            {
                output = Json::Value("InvalidCharacterStatistics");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesPhotonNotEnabledForTitle)
            {
                output = Json::Value("PhotonNotEnabledForTitle");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesPhotonApplicationNotFound)
            {
                output = Json::Value("PhotonApplicationNotFound");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesPhotonApplicationNotAssociatedWithTitle)
            {
                output = Json::Value("PhotonApplicationNotAssociatedWithTitle");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesInvalidEmailOrPassword)
            {
                output = Json::Value("InvalidEmailOrPassword");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesFacebookAPIError)
            {
                output = Json::Value("FacebookAPIError");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesInvalidContentType)
            {
                output = Json::Value("InvalidContentType");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesKeyLengthExceeded)
            {
                output = Json::Value("KeyLengthExceeded");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesDataLengthExceeded)
            {
                output = Json::Value("DataLengthExceeded");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesTooManyKeys)
            {
                output = Json::Value("TooManyKeys");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesFreeTierCannotHaveVirtualCurrency)
            {
                output = Json::Value("FreeTierCannotHaveVirtualCurrency");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesMissingAmazonSharedKey)
            {
                output = Json::Value("MissingAmazonSharedKey");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesAmazonValidationError)
            {
                output = Json::Value("AmazonValidationError");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesInvalidPSNIssuerId)
            {
                output = Json::Value("InvalidPSNIssuerId");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesPSNInaccessible)
            {
                output = Json::Value("PSNInaccessible");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesExpiredAuthToken)
            {
                output = Json::Value("ExpiredAuthToken");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesFailedToGetEntitlements)
            {
                output = Json::Value("FailedToGetEntitlements");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesFailedToConsumeEntitlement)
            {
                output = Json::Value("FailedToConsumeEntitlement");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesTradeAcceptingUserNotAllowed)
            {
                output = Json::Value("TradeAcceptingUserNotAllowed");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesTradeInventoryItemIsAssignedToCharacter)
            {
                output = Json::Value("TradeInventoryItemIsAssignedToCharacter");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesTradeInventoryItemIsBundle)
            {
                output = Json::Value("TradeInventoryItemIsBundle");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesTradeStatusNotValidForCancelling)
            {
                output = Json::Value("TradeStatusNotValidForCancelling");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesTradeStatusNotValidForAccepting)
            {
                output = Json::Value("TradeStatusNotValidForAccepting");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesTradeDoesNotExist)
            {
                output = Json::Value("TradeDoesNotExist");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesTradeCancelled)
            {
                output = Json::Value("TradeCancelled");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesTradeAlreadyFilled)
            {
                output = Json::Value("TradeAlreadyFilled");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesTradeWaitForStatusTimeout)
            {
                output = Json::Value("TradeWaitForStatusTimeout");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesTradeInventoryItemExpired)
            {
                output = Json::Value("TradeInventoryItemExpired");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesTradeMissingOfferedAndAcceptedItems)
            {
                output = Json::Value("TradeMissingOfferedAndAcceptedItems");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesTradeAcceptedItemIsBundle)
            {
                output = Json::Value("TradeAcceptedItemIsBundle");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesTradeAcceptedItemIsStackable)
            {
                output = Json::Value("TradeAcceptedItemIsStackable");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesTradeInventoryItemInvalidStatus)
            {
                output = Json::Value("TradeInventoryItemInvalidStatus");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesTradeAcceptedCatalogItemInvalid)
            {
                output = Json::Value("TradeAcceptedCatalogItemInvalid");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesTradeAllowedUsersInvalid)
            {
                output = Json::Value("TradeAllowedUsersInvalid");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesTradeInventoryItemDoesNotExist)
            {
                output = Json::Value("TradeInventoryItemDoesNotExist");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesTradeInventoryItemIsConsumed)
            {
                output = Json::Value("TradeInventoryItemIsConsumed");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesTradeInventoryItemIsStackable)
            {
                output = Json::Value("TradeInventoryItemIsStackable");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesTradeAcceptedItemsMismatch)
            {
                output = Json::Value("TradeAcceptedItemsMismatch");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesInvalidKongregateToken)
            {
                output = Json::Value("InvalidKongregateToken");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesFeatureNotConfiguredForTitle)
            {
                output = Json::Value("FeatureNotConfiguredForTitle");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesNoMatchingCatalogItemForReceipt)
            {
                output = Json::Value("NoMatchingCatalogItemForReceipt");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesInvalidCurrencyCode)
            {
                output = Json::Value("InvalidCurrencyCode");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesNoRealMoneyPriceForCatalogItem)
            {
                output = Json::Value("NoRealMoneyPriceForCatalogItem");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesTradeInventoryItemIsNotTradable)
            {
                output = Json::Value("TradeInventoryItemIsNotTradable");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesTradeAcceptedCatalogItemIsNotTradable)
            {
                output = Json::Value("TradeAcceptedCatalogItemIsNotTradable");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesUsersAlreadyFriends)
            {
                output = Json::Value("UsersAlreadyFriends");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesLinkedIdentifierAlreadyClaimed)
            {
                output = Json::Value("LinkedIdentifierAlreadyClaimed");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesCustomIdNotLinked)
            {
                output = Json::Value("CustomIdNotLinked");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesTotalDataSizeExceeded)
            {
                output = Json::Value("TotalDataSizeExceeded");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesDeleteKeyConflict)
            {
                output = Json::Value("DeleteKeyConflict");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesInvalidXboxLiveToken)
            {
                output = Json::Value("InvalidXboxLiveToken");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesExpiredXboxLiveToken)
            {
                output = Json::Value("ExpiredXboxLiveToken");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesResettableStatisticVersionRequired)
            {
                output = Json::Value("ResettableStatisticVersionRequired");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesNotAuthorizedByTitle)
            {
                output = Json::Value("NotAuthorizedByTitle");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesNoPartnerEnabled)
            {
                output = Json::Value("NoPartnerEnabled");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesInvalidPartnerResponse)
            {
                output = Json::Value("InvalidPartnerResponse");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesAPINotEnabledForGameServerAccess)
            {
                output = Json::Value("APINotEnabledForGameServerAccess");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesStatisticNotFound)
            {
                output = Json::Value("StatisticNotFound");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesStatisticNameConflict)
            {
                output = Json::Value("StatisticNameConflict");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesStatisticVersionClosedForWrites)
            {
                output = Json::Value("StatisticVersionClosedForWrites");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesStatisticVersionInvalid)
            {
                output = Json::Value("StatisticVersionInvalid");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesAPIClientRequestRateLimitExceeded)
            {
                output = Json::Value("APIClientRequestRateLimitExceeded");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesInvalidJSONContent)
            {
                output = Json::Value("InvalidJSONContent");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesInvalidDropTable)
            {
                output = Json::Value("InvalidDropTable");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesStatisticVersionAlreadyIncrementedForScheduledInterval)
            {
                output = Json::Value("StatisticVersionAlreadyIncrementedForScheduledInterval");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesStatisticCountLimitExceeded)
            {
                output = Json::Value("StatisticCountLimitExceeded");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesStatisticVersionIncrementRateExceeded)
            {
                output = Json::Value("StatisticVersionIncrementRateExceeded");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesContainerKeyInvalid)
            {
                output = Json::Value("ContainerKeyInvalid");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesCloudScriptExecutionTimeLimitExceeded)
            {
                output = Json::Value("CloudScriptExecutionTimeLimitExceeded");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesNoWritePermissionsForEvent)
            {
                output = Json::Value("NoWritePermissionsForEvent");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesCloudScriptFunctionArgumentSizeExceeded)
            {
                output = Json::Value("CloudScriptFunctionArgumentSizeExceeded");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesCloudScriptAPIRequestCountExceeded)
            {
                output = Json::Value("CloudScriptAPIRequestCountExceeded");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesCloudScriptAPIRequestError)
            {
                output = Json::Value("CloudScriptAPIRequestError");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesCloudScriptHTTPRequestError)
            {
                output = Json::Value("CloudScriptHTTPRequestError");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesInsufficientGuildRole)
            {
                output = Json::Value("InsufficientGuildRole");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesGuildNotFound)
            {
                output = Json::Value("GuildNotFound");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesOverLimit)
            {
                output = Json::Value("OverLimit");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesEventNotFound)
            {
                output = Json::Value("EventNotFound");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesInvalidEventField)
            {
                output = Json::Value("InvalidEventField");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesInvalidEventName)
            {
                output = Json::Value("InvalidEventName");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesCatalogNotConfigured)
            {
                output = Json::Value("CatalogNotConfigured");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesOperationNotSupportedForPlatform)
            {
                output = Json::Value("OperationNotSupportedForPlatform");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesSegmentNotFound)
            {
                output = Json::Value("SegmentNotFound");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesStoreNotFound)
            {
                output = Json::Value("StoreNotFound");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesInvalidStatisticName)
            {
                output = Json::Value("InvalidStatisticName");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesTitleNotQualifiedForLimit)
            {
                output = Json::Value("TitleNotQualifiedForLimit");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesInvalidServiceLimitLevel)
            {
                output = Json::Value("InvalidServiceLimitLevel");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesServiceLimitLevelInTransition)
            {
                output = Json::Value("ServiceLimitLevelInTransition");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesCouponAlreadyRedeemed)
            {
                output = Json::Value("CouponAlreadyRedeemed");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesGameServerBuildSizeLimitExceeded)
            {
                output = Json::Value("GameServerBuildSizeLimitExceeded");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesGameServerBuildCountLimitExceeded)
            {
                output = Json::Value("GameServerBuildCountLimitExceeded");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesVirtualCurrencyCountLimitExceeded)
            {
                output = Json::Value("VirtualCurrencyCountLimitExceeded");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesVirtualCurrencyCodeExists)
            {
                output = Json::Value("VirtualCurrencyCodeExists");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesTitleNewsItemCountLimitExceeded)
            {
                output = Json::Value("TitleNewsItemCountLimitExceeded");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesInvalidTwitchToken)
            {
                output = Json::Value("InvalidTwitchToken");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesTwitchResponseError)
            {
                output = Json::Value("TwitchResponseError");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesProfaneDisplayName)
            {
                output = Json::Value("ProfaneDisplayName");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesUserAlreadyAdded)
            {
                output = Json::Value("UserAlreadyAdded");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesInvalidVirtualCurrencyCode)
            {
                output = Json::Value("InvalidVirtualCurrencyCode");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesVirtualCurrencyCannotBeDeleted)
            {
                output = Json::Value("VirtualCurrencyCannotBeDeleted");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesIdentifierAlreadyClaimed)
            {
                output = Json::Value("IdentifierAlreadyClaimed");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesIdentifierNotLinked)
            {
                output = Json::Value("IdentifierNotLinked");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesInvalidContinuationToken)
            {
                output = Json::Value("InvalidContinuationToken");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesExpiredContinuationToken)
            {
                output = Json::Value("ExpiredContinuationToken");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesInvalidSegment)
            {
                output = Json::Value("InvalidSegment");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesInvalidSessionId)
            {
                output = Json::Value("InvalidSessionId");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesSessionLogNotFound)
            {
                output = Json::Value("SessionLogNotFound");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesInvalidSearchTerm)
            {
                output = Json::Value("InvalidSearchTerm");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesTwoFactorAuthenticationTokenRequired)
            {
                output = Json::Value("TwoFactorAuthenticationTokenRequired");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesGameServerHostCountLimitExceeded)
            {
                output = Json::Value("GameServerHostCountLimitExceeded");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesPlayerTagCountLimitExceeded)
            {
                output = Json::Value("PlayerTagCountLimitExceeded");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesRequestAlreadyRunning)
            {
                output = Json::Value("RequestAlreadyRunning");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesActionGroupNotFound)
            {
                output = Json::Value("ActionGroupNotFound");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesMaximumSegmentBulkActionJobsRunning)
            {
                output = Json::Value("MaximumSegmentBulkActionJobsRunning");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesNoActionsOnPlayersInSegmentJob)
            {
                output = Json::Value("NoActionsOnPlayersInSegmentJob");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesDuplicateStatisticName)
            {
                output = Json::Value("DuplicateStatisticName");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesScheduledTaskNameConflict)
            {
                output = Json::Value("ScheduledTaskNameConflict");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesScheduledTaskCreateConflict)
            {
                output = Json::Value("ScheduledTaskCreateConflict");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesInvalidScheduledTaskName)
            {
                output = Json::Value("InvalidScheduledTaskName");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesInvalidTaskSchedule)
            {
                output = Json::Value("InvalidTaskSchedule");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesSteamNotEnabledForTitle)
            {
                output = Json::Value("SteamNotEnabledForTitle");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesLimitNotAnUpgradeOption)
            {
                output = Json::Value("LimitNotAnUpgradeOption");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesNoSecretKeyEnabledForCloudScript)
            {
                output = Json::Value("NoSecretKeyEnabledForCloudScript");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesTaskNotFound)
            {
                output = Json::Value("TaskNotFound");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesTaskInstanceNotFound)
            {
                output = Json::Value("TaskInstanceNotFound");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesInvalidIdentityProviderId)
            {
                output = Json::Value("InvalidIdentityProviderId");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesMisconfiguredIdentityProvider)
            {
                output = Json::Value("MisconfiguredIdentityProvider");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesInvalidScheduledTaskType)
            {
                output = Json::Value("InvalidScheduledTaskType");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesBillingInformationRequired)
            {
                output = Json::Value("BillingInformationRequired");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesLimitedEditionItemUnavailable)
            {
                output = Json::Value("LimitedEditionItemUnavailable");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesInvalidAdPlacementAndReward)
            {
                output = Json::Value("InvalidAdPlacementAndReward");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesAllAdPlacementViewsAlreadyConsumed)
            {
                output = Json::Value("AllAdPlacementViewsAlreadyConsumed");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesGoogleOAuthNotConfiguredForTitle)
            {
                output = Json::Value("GoogleOAuthNotConfiguredForTitle");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesGoogleOAuthError)
            {
                output = Json::Value("GoogleOAuthError");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesUserNotFriend)
            {
                output = Json::Value("UserNotFriend");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesInvalidSignature)
            {
                output = Json::Value("InvalidSignature");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesInvalidPublicKey)
            {
                output = Json::Value("InvalidPublicKey");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesGoogleOAuthNoIdTokenIncludedInResponse)
            {
                output = Json::Value("GoogleOAuthNoIdTokenIncludedInResponse");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesStatisticUpdateInProgress)
            {
                output = Json::Value("StatisticUpdateInProgress");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesLeaderboardVersionNotAvailable)
            {
                output = Json::Value("LeaderboardVersionNotAvailable");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesStatisticAlreadyHasPrizeTable)
            {
                output = Json::Value("StatisticAlreadyHasPrizeTable");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesPrizeTableHasOverlappingRanks)
            {
                output = Json::Value("PrizeTableHasOverlappingRanks");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesPrizeTableHasMissingRanks)
            {
                output = Json::Value("PrizeTableHasMissingRanks");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesPrizeTableRankStartsAtZero)
            {
                output = Json::Value("PrizeTableRankStartsAtZero");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesInvalidStatistic)
            {
                output = Json::Value("InvalidStatistic");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesExpressionParseFailure)
            {
                output = Json::Value("ExpressionParseFailure");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesExpressionInvokeFailure)
            {
                output = Json::Value("ExpressionInvokeFailure");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesExpressionTooLong)
            {
                output = Json::Value("ExpressionTooLong");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesDataUpdateRateExceeded)
            {
                output = Json::Value("DataUpdateRateExceeded");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesRestrictedEmailDomain)
            {
                output = Json::Value("RestrictedEmailDomain");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesEncryptionKeyDisabled)
            {
                output = Json::Value("EncryptionKeyDisabled");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesEncryptionKeyMissing)
            {
                output = Json::Value("EncryptionKeyMissing");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesEncryptionKeyBroken)
            {
                output = Json::Value("EncryptionKeyBroken");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesNoSharedSecretKeyConfigured)
            {
                output = Json::Value("NoSharedSecretKeyConfigured");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesSecretKeyNotFound)
            {
                output = Json::Value("SecretKeyNotFound");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesPlayerSecretAlreadyConfigured)
            {
                output = Json::Value("PlayerSecretAlreadyConfigured");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesAPIRequestsDisabledForTitle)
            {
                output = Json::Value("APIRequestsDisabledForTitle");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesInvalidSharedSecretKey)
            {
                output = Json::Value("InvalidSharedSecretKey");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesPrizeTableHasNoRanks)
            {
                output = Json::Value("PrizeTableHasNoRanks");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesProfileDoesNotExist)
            {
                output = Json::Value("ProfileDoesNotExist");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesContentS3OriginBucketNotConfigured)
            {
                output = Json::Value("ContentS3OriginBucketNotConfigured");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesInvalidEnvironmentForReceipt)
            {
                output = Json::Value("InvalidEnvironmentForReceipt");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesEncryptedRequestNotAllowed)
            {
                output = Json::Value("EncryptedRequestNotAllowed");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesSignedRequestNotAllowed)
            {
                output = Json::Value("SignedRequestNotAllowed");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesRequestViewConstraintParamsNotAllowed)
            {
                output = Json::Value("RequestViewConstraintParamsNotAllowed");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesBadPartnerConfiguration)
            {
                output = Json::Value("BadPartnerConfiguration");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesXboxBPCertificateFailure)
            {
                output = Json::Value("XboxBPCertificateFailure");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesXboxXASSExchangeFailure)
            {
                output = Json::Value("XboxXASSExchangeFailure");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesInvalidEntityId)
            {
                output = Json::Value("InvalidEntityId");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesStatisticValueAggregationOverflow)
            {
                output = Json::Value("StatisticValueAggregationOverflow");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesEmailMessageFromAddressIsMissing)
            {
                output = Json::Value("EmailMessageFromAddressIsMissing");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesEmailMessageToAddressIsMissing)
            {
                output = Json::Value("EmailMessageToAddressIsMissing");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesSmtpServerAuthenticationError)
            {
                output = Json::Value("SmtpServerAuthenticationError");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesSmtpServerLimitExceeded)
            {
                output = Json::Value("SmtpServerLimitExceeded");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesSmtpServerInsufficientStorage)
            {
                output = Json::Value("SmtpServerInsufficientStorage");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesSmtpServerCommunicationError)
            {
                output = Json::Value("SmtpServerCommunicationError");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesSmtpServerGeneralFailure)
            {
                output = Json::Value("SmtpServerGeneralFailure");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesEmailClientTimeout)
            {
                output = Json::Value("EmailClientTimeout");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesEmailClientCanceledTask)
            {
                output = Json::Value("EmailClientCanceledTask");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesEmailTemplateMissing)
            {
                output = Json::Value("EmailTemplateMissing");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesInvalidHostForTitleId)
            {
                output = Json::Value("InvalidHostForTitleId");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesEmailConfirmationTokenDoesNotExist)
            {
                output = Json::Value("EmailConfirmationTokenDoesNotExist");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesEmailConfirmationTokenExpired)
            {
                output = Json::Value("EmailConfirmationTokenExpired");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesAccountDeleted)
            {
                output = Json::Value("AccountDeleted");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesPlayerSecretNotConfigured)
            {
                output = Json::Value("PlayerSecretNotConfigured");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesInvalidSignatureTime)
            {
                output = Json::Value("InvalidSignatureTime");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesNoContactEmailAddressFound)
            {
                output = Json::Value("NoContactEmailAddressFound");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesInvalidAuthToken)
            {
                output = Json::Value("InvalidAuthToken");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesAuthTokenDoesNotExist)
            {
                output = Json::Value("AuthTokenDoesNotExist");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesAuthTokenExpired)
            {
                output = Json::Value("AuthTokenExpired");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesAuthTokenAlreadyUsedToResetPassword)
            {
                output = Json::Value("AuthTokenAlreadyUsedToResetPassword");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesMembershipNameTooLong)
            {
                output = Json::Value("MembershipNameTooLong");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesMembershipNotFound)
            {
                output = Json::Value("MembershipNotFound");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesGoogleServiceAccountInvalid)
            {
                output = Json::Value("GoogleServiceAccountInvalid");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesGoogleServiceAccountParseFailure)
            {
                output = Json::Value("GoogleServiceAccountParseFailure");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesEntityTokenMissing)
            {
                output = Json::Value("EntityTokenMissing");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesEntityTokenInvalid)
            {
                output = Json::Value("EntityTokenInvalid");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesEntityTokenExpired)
            {
                output = Json::Value("EntityTokenExpired");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesEntityTokenRevoked)
            {
                output = Json::Value("EntityTokenRevoked");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesInvalidProductForSubscription)
            {
                output = Json::Value("InvalidProductForSubscription");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesXboxInaccessible)
            {
                output = Json::Value("XboxInaccessible");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesSubscriptionAlreadyTaken)
            {
                output = Json::Value("SubscriptionAlreadyTaken");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesSmtpAddonNotEnabled)
            {
                output = Json::Value("SmtpAddonNotEnabled");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesAPIConcurrentRequestLimitExceeded)
            {
                output = Json::Value("APIConcurrentRequestLimitExceeded");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesXboxRejectedXSTSExchangeRequest)
            {
                output = Json::Value("XboxRejectedXSTSExchangeRequest");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesVariableNotDefined)
            {
                output = Json::Value("VariableNotDefined");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesTemplateVersionNotDefined)
            {
                output = Json::Value("TemplateVersionNotDefined");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesFileTooLarge)
            {
                output = Json::Value("FileTooLarge");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesTitleDeleted)
            {
                output = Json::Value("TitleDeleted");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesTitleContainsUserAccounts)
            {
                output = Json::Value("TitleContainsUserAccounts");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesTitleDeletionPlayerCleanupFailure)
            {
                output = Json::Value("TitleDeletionPlayerCleanupFailure");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesEntityFileOperationPending)
            {
                output = Json::Value("EntityFileOperationPending");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesNoEntityFileOperationPending)
            {
                output = Json::Value("NoEntityFileOperationPending");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesEntityProfileVersionMismatch)
            {
                output = Json::Value("EntityProfileVersionMismatch");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesTemplateVersionTooOld)
            {
                output = Json::Value("TemplateVersionTooOld");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesMembershipDefinitionInUse)
            {
                output = Json::Value("MembershipDefinitionInUse");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesPaymentPageNotConfigured)
            {
                output = Json::Value("PaymentPageNotConfigured");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesFailedLoginAttemptRateLimitExceeded)
            {
                output = Json::Value("FailedLoginAttemptRateLimitExceeded");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesEntityBlockedByGroup)
            {
                output = Json::Value("EntityBlockedByGroup");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesRoleDoesNotExist)
            {
                output = Json::Value("RoleDoesNotExist");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesEntityIsAlreadyMember)
            {
                output = Json::Value("EntityIsAlreadyMember");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesDuplicateRoleId)
            {
                output = Json::Value("DuplicateRoleId");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesGroupInvitationNotFound)
            {
                output = Json::Value("GroupInvitationNotFound");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesGroupApplicationNotFound)
            {
                output = Json::Value("GroupApplicationNotFound");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesOutstandingInvitationAcceptedInstead)
            {
                output = Json::Value("OutstandingInvitationAcceptedInstead");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesOutstandingApplicationAcceptedInstead)
            {
                output = Json::Value("OutstandingApplicationAcceptedInstead");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesRoleIsGroupDefaultMember)
            {
                output = Json::Value("RoleIsGroupDefaultMember");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesRoleIsGroupAdmin)
            {
                output = Json::Value("RoleIsGroupAdmin");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesRoleNameNotAvailable)
            {
                output = Json::Value("RoleNameNotAvailable");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesGroupNameNotAvailable)
            {
                output = Json::Value("GroupNameNotAvailable");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesEmailReportAlreadySent)
            {
                output = Json::Value("EmailReportAlreadySent");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesEmailReportRecipientBlacklisted)
            {
                output = Json::Value("EmailReportRecipientBlacklisted");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesEventNamespaceNotAllowed)
            {
                output = Json::Value("EventNamespaceNotAllowed");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesEventEntityNotAllowed)
            {
                output = Json::Value("EventEntityNotAllowed");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesInvalidEntityType)
            {
                output = Json::Value("InvalidEntityType");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesNullTokenResultFromAad)
            {
                output = Json::Value("NullTokenResultFromAad");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesInvalidTokenResultFromAad)
            {
                output = Json::Value("InvalidTokenResultFromAad");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesNoValidCertificateForAad)
            {
                output = Json::Value("NoValidCertificateForAad");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesInvalidCertificateForAad)
            {
                output = Json::Value("InvalidCertificateForAad");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesDuplicateDropTableId)
            {
                output = Json::Value("DuplicateDropTableId");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesMultiplayerServerError)
            {
                output = Json::Value("MultiplayerServerError");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesMultiplayerServerTooManyRequests)
            {
                output = Json::Value("MultiplayerServerTooManyRequests");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesMultiplayerServerNoContent)
            {
                output = Json::Value("MultiplayerServerNoContent");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesMultiplayerServerBadRequest)
            {
                output = Json::Value("MultiplayerServerBadRequest");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesMultiplayerServerUnauthorized)
            {
                output = Json::Value("MultiplayerServerUnauthorized");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesMultiplayerServerForbidden)
            {
                output = Json::Value("MultiplayerServerForbidden");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesMultiplayerServerNotFound)
            {
                output = Json::Value("MultiplayerServerNotFound");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesMultiplayerServerConflict)
            {
                output = Json::Value("MultiplayerServerConflict");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesMultiplayerServerInternalServerError)
            {
                output = Json::Value("MultiplayerServerInternalServerError");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesMultiplayerServerUnavailable)
            {
                output = Json::Value("MultiplayerServerUnavailable");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesExplicitContentDetected)
            {
                output = Json::Value("ExplicitContentDetected");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesPIIContentDetected)
            {
                output = Json::Value("PIIContentDetected");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesInvalidScheduledTaskParameter)
            {
                output = Json::Value("InvalidScheduledTaskParameter");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesPerEntityEventRateLimitExceeded)
            {
                output = Json::Value("PerEntityEventRateLimitExceeded");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesTitleDefaultLanguageNotSet)
            {
                output = Json::Value("TitleDefaultLanguageNotSet");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesEmailTemplateMissingDefaultVersion)
            {
                output = Json::Value("EmailTemplateMissingDefaultVersion");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesFacebookInstantGamesIdNotLinked)
            {
                output = Json::Value("FacebookInstantGamesIdNotLinked");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesInvalidFacebookInstantGamesSignature)
            {
                output = Json::Value("InvalidFacebookInstantGamesSignature");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesFacebookInstantGamesAuthNotConfiguredForTitle)
            {
                output = Json::Value("FacebookInstantGamesAuthNotConfiguredForTitle");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesEntityProfileConstraintValidationFailed)
            {
                output = Json::Value("EntityProfileConstraintValidationFailed");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesTelemetryIngestionKeyPending)
            {
                output = Json::Value("TelemetryIngestionKeyPending");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesTelemetryIngestionKeyNotFound)
            {
                output = Json::Value("TelemetryIngestionKeyNotFound");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesStatisticChildNameInvalid)
            {
                output = Json::Value("StatisticChildNameInvalid");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesDataIntegrityError)
            {
                output = Json::Value("DataIntegrityError");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesVirtualCurrencyCannotBeSetToOlderVersion)
            {
                output = Json::Value("VirtualCurrencyCannotBeSetToOlderVersion");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesVirtualCurrencyMustBeWithinIntegerRange)
            {
                output = Json::Value("VirtualCurrencyMustBeWithinIntegerRange");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesEmailTemplateInvalidSyntax)
            {
                output = Json::Value("EmailTemplateInvalidSyntax");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesEmailTemplateMissingCallback)
            {
                output = Json::Value("EmailTemplateMissingCallback");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesPushNotificationTemplateInvalidPayload)
            {
                output = Json::Value("PushNotificationTemplateInvalidPayload");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesInvalidLocalizedPushNotificationLanguage)
            {
                output = Json::Value("InvalidLocalizedPushNotificationLanguage");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesMissingLocalizedPushNotificationMessage)
            {
                output = Json::Value("MissingLocalizedPushNotificationMessage");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesPushNotificationTemplateMissingPlatformPayload)
            {
                output = Json::Value("PushNotificationTemplateMissingPlatformPayload");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesPushNotificationTemplatePayloadContainsInvalidJson)
            {
                output = Json::Value("PushNotificationTemplatePayloadContainsInvalidJson");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesPushNotificationTemplateContainsInvalidIosPayload)
            {
                output = Json::Value("PushNotificationTemplateContainsInvalidIosPayload");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesPushNotificationTemplateContainsInvalidAndroidPayload)
            {
                output = Json::Value("PushNotificationTemplateContainsInvalidAndroidPayload");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesPushNotificationTemplateIosPayloadMissingNotificationBody)
            {
                output = Json::Value("PushNotificationTemplateIosPayloadMissingNotificationBody");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesPushNotificationTemplateAndroidPayloadMissingNotificationBody)
            {
                output = Json::Value("PushNotificationTemplateAndroidPayloadMissingNotificationBody");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesPushNotificationTemplateNotFound)
            {
                output = Json::Value("PushNotificationTemplateNotFound");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesPushNotificationTemplateMissingDefaultVersion)
            {
                output = Json::Value("PushNotificationTemplateMissingDefaultVersion");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesPushNotificationTemplateInvalidSyntax)
            {
                output = Json::Value("PushNotificationTemplateInvalidSyntax");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesPushNotificationTemplateNoCustomPayloadForV1)
            {
                output = Json::Value("PushNotificationTemplateNoCustomPayloadForV1");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesNoLeaderboardForStatistic)
            {
                output = Json::Value("NoLeaderboardForStatistic");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesTitleNewsMissingDefaultLanguage)
            {
                output = Json::Value("TitleNewsMissingDefaultLanguage");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesTitleNewsNotFound)
            {
                output = Json::Value("TitleNewsNotFound");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesTitleNewsDuplicateLanguage)
            {
                output = Json::Value("TitleNewsDuplicateLanguage");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesTitleNewsMissingTitleOrBody)
            {
                output = Json::Value("TitleNewsMissingTitleOrBody");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesTitleNewsInvalidLanguage)
            {
                output = Json::Value("TitleNewsInvalidLanguage");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesEmailRecipientBlacklisted)
            {
                output = Json::Value("EmailRecipientBlacklisted");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesInvalidGameCenterAuthRequest)
            {
                output = Json::Value("InvalidGameCenterAuthRequest");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesGameCenterAuthenticationFailed)
            {
                output = Json::Value("GameCenterAuthenticationFailed");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesCannotEnablePartiesForTitle)
            {
                output = Json::Value("CannotEnablePartiesForTitle");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesPartyError)
            {
                output = Json::Value("PartyError");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesPartyRequests)
            {
                output = Json::Value("PartyRequests");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesPartyNoContent)
            {
                output = Json::Value("PartyNoContent");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesPartyBadRequest)
            {
                output = Json::Value("PartyBadRequest");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesPartyUnauthorized)
            {
                output = Json::Value("PartyUnauthorized");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesPartyForbidden)
            {
                output = Json::Value("PartyForbidden");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesPartyNotFound)
            {
                output = Json::Value("PartyNotFound");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesPartyConflict)
            {
                output = Json::Value("PartyConflict");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesPartyInternalServerError)
            {
                output = Json::Value("PartyInternalServerError");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesPartyUnavailable)
            {
                output = Json::Value("PartyUnavailable");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesPartyTooManyRequests)
            {
                output = Json::Value("PartyTooManyRequests");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesPushNotificationTemplateMissingName)
            {
                output = Json::Value("PushNotificationTemplateMissingName");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesCannotEnableMultiplayerServersForTitle)
            {
                output = Json::Value("CannotEnableMultiplayerServersForTitle");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesWriteAttemptedDuringExport)
            {
                output = Json::Value("WriteAttemptedDuringExport");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesMultiplayerServerTitleQuotaCoresExceeded)
            {
                output = Json::Value("MultiplayerServerTitleQuotaCoresExceeded");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesAutomationRuleNotFound)
            {
                output = Json::Value("AutomationRuleNotFound");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesEntityAPIKeyLimitExceeded)
            {
                output = Json::Value("EntityAPIKeyLimitExceeded");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesEntityAPIKeyNotFound)
            {
                output = Json::Value("EntityAPIKeyNotFound");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesEntityAPIKeyOrSecretInvalid)
            {
                output = Json::Value("EntityAPIKeyOrSecretInvalid");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesEconomyServiceUnavailable)
            {
                output = Json::Value("EconomyServiceUnavailable");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesEconomyServiceInternalError)
            {
                output = Json::Value("EconomyServiceInternalError");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesQueryRateLimitExceeded)
            {
                output = Json::Value("QueryRateLimitExceeded");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesEntityAPIKeyCreationDisabledForEntity)
            {
                output = Json::Value("EntityAPIKeyCreationDisabledForEntity");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesForbiddenByEntityPolicy)
            {
                output = Json::Value("ForbiddenByEntityPolicy");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesUpdateInventoryRateLimitExceeded)
            {
                output = Json::Value("UpdateInventoryRateLimitExceeded");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesStudioCreationRateLimited)
            {
                output = Json::Value("StudioCreationRateLimited");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesStudioCreationInProgress)
            {
                output = Json::Value("StudioCreationInProgress");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesDuplicateStudioName)
            {
                output = Json::Value("DuplicateStudioName");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesStudioNotFound)
            {
                output = Json::Value("StudioNotFound");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesStudioDeleted)
            {
                output = Json::Value("StudioDeleted");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesStudioDeactivated)
            {
                output = Json::Value("StudioDeactivated");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesStudioActivated)
            {
                output = Json::Value("StudioActivated");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesTitleCreationRateLimited)
            {
                output = Json::Value("TitleCreationRateLimited");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesTitleCreationInProgress)
            {
                output = Json::Value("TitleCreationInProgress");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesDuplicateTitleName)
            {
                output = Json::Value("DuplicateTitleName");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesTitleActivationRateLimited)
            {
                output = Json::Value("TitleActivationRateLimited");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesTitleActivationInProgress)
            {
                output = Json::Value("TitleActivationInProgress");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesTitleDeactivated)
            {
                output = Json::Value("TitleDeactivated");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesTitleActivated)
            {
                output = Json::Value("TitleActivated");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesCloudScriptAzureFunctionsExecutionTimeLimitExceeded)
            {
                output = Json::Value("CloudScriptAzureFunctionsExecutionTimeLimitExceeded");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesCloudScriptAzureFunctionsArgumentSizeExceeded)
            {
                output = Json::Value("CloudScriptAzureFunctionsArgumentSizeExceeded");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesCloudScriptAzureFunctionsReturnSizeExceeded)
            {
                output = Json::Value("CloudScriptAzureFunctionsReturnSizeExceeded");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesCloudScriptAzureFunctionsHTTPRequestError)
            {
                output = Json::Value("CloudScriptAzureFunctionsHTTPRequestError");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesVirtualCurrencyBetaGetError)
            {
                output = Json::Value("VirtualCurrencyBetaGetError");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesVirtualCurrencyBetaCreateError)
            {
                output = Json::Value("VirtualCurrencyBetaCreateError");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesVirtualCurrencyBetaInitialDepositSaveError)
            {
                output = Json::Value("VirtualCurrencyBetaInitialDepositSaveError");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesVirtualCurrencyBetaSaveError)
            {
                output = Json::Value("VirtualCurrencyBetaSaveError");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesVirtualCurrencyBetaDeleteError)
            {
                output = Json::Value("VirtualCurrencyBetaDeleteError");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesVirtualCurrencyBetaRestoreError)
            {
                output = Json::Value("VirtualCurrencyBetaRestoreError");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesVirtualCurrencyBetaSaveConflict)
            {
                output = Json::Value("VirtualCurrencyBetaSaveConflict");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesVirtualCurrencyBetaUpdateError)
            {
                output = Json::Value("VirtualCurrencyBetaUpdateError");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesInsightsManagementDatabaseNotFound)
            {
                output = Json::Value("InsightsManagementDatabaseNotFound");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesInsightsManagementOperationNotFound)
            {
                output = Json::Value("InsightsManagementOperationNotFound");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesInsightsManagementErrorPendingOperationExists)
            {
                output = Json::Value("InsightsManagementErrorPendingOperationExists");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesInsightsManagementSetPerformanceLevelInvalidParameter)
            {
                output = Json::Value("InsightsManagementSetPerformanceLevelInvalidParameter");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesInsightsManagementSetStorageRetentionInvalidParameter)
            {
                output = Json::Value("InsightsManagementSetStorageRetentionInvalidParameter");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesInsightsManagementGetStorageUsageInvalidParameter)
            {
                output = Json::Value("InsightsManagementGetStorageUsageInvalidParameter");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesInsightsManagementGetOperationStatusInvalidParameter)
            {
                output = Json::Value("InsightsManagementGetOperationStatusInvalidParameter");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesDuplicatePurchaseTransactionId)
            {
                output = Json::Value("DuplicatePurchaseTransactionId");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesEvaluationModePlayerCountExceeded)
            {
                output = Json::Value("EvaluationModePlayerCountExceeded");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesGetPlayersInSegmentRateLimitExceeded)
            {
                output = Json::Value("GetPlayersInSegmentRateLimitExceeded");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesCloudScriptFunctionNameSizeExceeded)
            {
                output = Json::Value("CloudScriptFunctionNameSizeExceeded");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesInsightsManagementTitleInEvaluationMode)
            {
                output = Json::Value("InsightsManagementTitleInEvaluationMode");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesCloudScriptAzureFunctionsQueueRequestError)
            {
                output = Json::Value("CloudScriptAzureFunctionsQueueRequestError");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesEvaluationModeTitleCountExceeded)
            {
                output = Json::Value("EvaluationModeTitleCountExceeded");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesInsightsManagementTitleNotInFlight)
            {
                output = Json::Value("InsightsManagementTitleNotInFlight");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesLimitNotFound)
            {
                output = Json::Value("LimitNotFound");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesLimitNotAvailableViaAPI)
            {
                output = Json::Value("LimitNotAvailableViaAPI");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesInsightsManagementSetStorageRetentionBelowMinimum)
            {
                output = Json::Value("InsightsManagementSetStorageRetentionBelowMinimum");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesInsightsManagementSetStorageRetentionAboveMaximum)
            {
                output = Json::Value("InsightsManagementSetStorageRetentionAboveMaximum");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesAppleNotEnabledForTitle)
            {
                output = Json::Value("AppleNotEnabledForTitle");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesInsightsManagementNewActiveEventExportLimitInvalid)
            {
                output = Json::Value("InsightsManagementNewActiveEventExportLimitInvalid");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesInsightsManagementSetPerformanceRateLimited)
            {
                output = Json::Value("InsightsManagementSetPerformanceRateLimited");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesPartyRequestsThrottledFromRateLimiter)
            {
                output = Json::Value("PartyRequestsThrottledFromRateLimiter");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesXboxServiceTooManyRequests)
            {
                output = Json::Value("XboxServiceTooManyRequests");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesNintendoSwitchNotEnabledForTitle)
            {
                output = Json::Value("NintendoSwitchNotEnabledForTitle");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesRequestMultiplayerServersThrottledFromRateLimiter)
            {
                output = Json::Value("RequestMultiplayerServersThrottledFromRateLimiter");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesTitleDataOverrideNotFound)
            {
                output = Json::Value("TitleDataOverrideNotFound");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesDuplicateKeys)
            {
                output = Json::Value("DuplicateKeys");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesWasNotCreatedWithCloudRoot)
            {
                output = Json::Value("WasNotCreatedWithCloudRoot");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesLegacyMultiplayerServersDeprecated)
            {
                output = Json::Value("LegacyMultiplayerServersDeprecated");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesMatchmakingEntityInvalid)
            {
                output = Json::Value("MatchmakingEntityInvalid");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesMatchmakingPlayerAttributesInvalid)
            {
                output = Json::Value("MatchmakingPlayerAttributesInvalid");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesMatchmakingQueueNotFound)
            {
                output = Json::Value("MatchmakingQueueNotFound");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesMatchmakingMatchNotFound)
            {
                output = Json::Value("MatchmakingMatchNotFound");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesMatchmakingTicketNotFound)
            {
                output = Json::Value("MatchmakingTicketNotFound");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesMatchmakingAlreadyJoinedTicket)
            {
                output = Json::Value("MatchmakingAlreadyJoinedTicket");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesMatchmakingTicketAlreadyCompleted)
            {
                output = Json::Value("MatchmakingTicketAlreadyCompleted");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesMatchmakingQueueConfigInvalid)
            {
                output = Json::Value("MatchmakingQueueConfigInvalid");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesMatchmakingMemberProfileInvalid)
            {
                output = Json::Value("MatchmakingMemberProfileInvalid");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesNintendoSwitchDeviceIdNotLinked)
            {
                output = Json::Value("NintendoSwitchDeviceIdNotLinked");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesMatchmakingNotEnabled)
            {
                output = Json::Value("MatchmakingNotEnabled");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesMatchmakingPlayerAttributesTooLarge)
            {
                output = Json::Value("MatchmakingPlayerAttributesTooLarge");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesMatchmakingNumberOfPlayersInTicketTooLarge)
            {
                output = Json::Value("MatchmakingNumberOfPlayersInTicketTooLarge");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesMatchmakingAttributeInvalid)
            {
                output = Json::Value("MatchmakingAttributeInvalid");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesMatchmakingPlayerHasNotJoinedTicket)
            {
                output = Json::Value("MatchmakingPlayerHasNotJoinedTicket");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesMatchmakingRateLimitExceeded)
            {
                output = Json::Value("MatchmakingRateLimitExceeded");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesMatchmakingTicketMembershipLimitExceeded)
            {
                output = Json::Value("MatchmakingTicketMembershipLimitExceeded");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesMatchmakingUnauthorized)
            {
                output = Json::Value("MatchmakingUnauthorized");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesMatchmakingQueueLimitExceeded)
            {
                output = Json::Value("MatchmakingQueueLimitExceeded");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesMatchmakingRequestTypeMismatch)
            {
                output = Json::Value("MatchmakingRequestTypeMismatch");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesMatchmakingBadRequest)
            {
                output = Json::Value("MatchmakingBadRequest");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesTitleConfigNotFound)
            {
                output = Json::Value("TitleConfigNotFound");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesTitleConfigUpdateConflict)
            {
                output = Json::Value("TitleConfigUpdateConflict");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesTitleConfigSerializationError)
            {
                output = Json::Value("TitleConfigSerializationError");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesCatalogEntityInvalid)
            {
                output = Json::Value("CatalogEntityInvalid");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesCatalogTitleIdMissing)
            {
                output = Json::Value("CatalogTitleIdMissing");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesCatalogPlayerIdMissing)
            {
                output = Json::Value("CatalogPlayerIdMissing");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesCatalogClientIdentityInvalid)
            {
                output = Json::Value("CatalogClientIdentityInvalid");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesCatalogOneOrMoreFilesInvalid)
            {
                output = Json::Value("CatalogOneOrMoreFilesInvalid");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesCatalogItemMetadataInvalid)
            {
                output = Json::Value("CatalogItemMetadataInvalid");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesCatalogItemIdInvalid)
            {
                output = Json::Value("CatalogItemIdInvalid");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesCatalogSearchParameterInvalid)
            {
                output = Json::Value("CatalogSearchParameterInvalid");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesCatalogFeatureDisabled)
            {
                output = Json::Value("CatalogFeatureDisabled");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesCatalogConfigInvalid)
            {
                output = Json::Value("CatalogConfigInvalid");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesCatalogUnauthorized)
            {
                output = Json::Value("CatalogUnauthorized");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesCatalogItemTypeInvalid)
            {
                output = Json::Value("CatalogItemTypeInvalid");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesCatalogBadRequest)
            {
                output = Json::Value("CatalogBadRequest");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesCatalogTooManyRequests)
            {
                output = Json::Value("CatalogTooManyRequests");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesExportInvalidStatusUpdate)
            {
                output = Json::Value("ExportInvalidStatusUpdate");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesExportInvalidPrefix)
            {
                output = Json::Value("ExportInvalidPrefix");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesExportBlobContainerDoesNotExist)
            {
                output = Json::Value("ExportBlobContainerDoesNotExist");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesExportNotFound)
            {
                output = Json::Value("ExportNotFound");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesExportCouldNotUpdate)
            {
                output = Json::Value("ExportCouldNotUpdate");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesExportInvalidStorageType)
            {
                output = Json::Value("ExportInvalidStorageType");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesExportAmazonBucketDoesNotExist)
            {
                output = Json::Value("ExportAmazonBucketDoesNotExist");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesExportInvalidBlobStorage)
            {
                output = Json::Value("ExportInvalidBlobStorage");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesExportKustoException)
            {
                output = Json::Value("ExportKustoException");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesExportKustoConnectionFailed)
            {
                output = Json::Value("ExportKustoConnectionFailed");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesExportUnknownError)
            {
                output = Json::Value("ExportUnknownError");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesExportCantEditPendingExport)
            {
                output = Json::Value("ExportCantEditPendingExport");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesExportLimitExports)
            {
                output = Json::Value("ExportLimitExports");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesExportLimitEvents)
            {
                output = Json::Value("ExportLimitEvents");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesExportInvalidPartitionStatusModification)
            {
                output = Json::Value("ExportInvalidPartitionStatusModification");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesExportCouldNotCreate)
            {
                output = Json::Value("ExportCouldNotCreate");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesExportNoBackingDatabaseFound)
            {
                output = Json::Value("ExportNoBackingDatabaseFound");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesExportCouldNotDelete)
            {
                output = Json::Value("ExportCouldNotDelete");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesExportCannotDetermineEventQuery)
            {
                output = Json::Value("ExportCannotDetermineEventQuery");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesExportInvalidQuerySchemaModification)
            {
                output = Json::Value("ExportInvalidQuerySchemaModification");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesExportQuerySchemaMissingRequiredColumns)
            {
                output = Json::Value("ExportQuerySchemaMissingRequiredColumns");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesExportCannotParseQuery)
            {
                output = Json::Value("ExportCannotParseQuery");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesExportControlCommandsNotAllowed)
            {
                output = Json::Value("ExportControlCommandsNotAllowed");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesExportQueryMissingTableReference)
            {
                output = Json::Value("ExportQueryMissingTableReference");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesTitleNotEnabledForParty)
            {
                output = Json::Value("TitleNotEnabledForParty");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesPartyVersionNotFound)
            {
                output = Json::Value("PartyVersionNotFound");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesMultiplayerServerBuildReferencedByMatchmakingQueue)
            {
                output = Json::Value("MultiplayerServerBuildReferencedByMatchmakingQueue");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesExperimentationExperimentStopped)
            {
                output = Json::Value("ExperimentationExperimentStopped");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesExperimentationExperimentRunning)
            {
                output = Json::Value("ExperimentationExperimentRunning");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesExperimentationExperimentNotFound)
            {
                output = Json::Value("ExperimentationExperimentNotFound");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesExperimentationExperimentNeverStarted)
            {
                output = Json::Value("ExperimentationExperimentNeverStarted");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesExperimentationExperimentDeleted)
            {
                output = Json::Value("ExperimentationExperimentDeleted");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesExperimentationClientTimeout)
            {
                output = Json::Value("ExperimentationClientTimeout");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesExperimentationInvalidVariantConfiguration)
            {
                output = Json::Value("ExperimentationInvalidVariantConfiguration");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesExperimentationInvalidVariableConfiguration)
            {
                output = Json::Value("ExperimentationInvalidVariableConfiguration");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesExperimentInvalidId)
            {
                output = Json::Value("ExperimentInvalidId");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesExperimentationNoScorecard)
            {
                output = Json::Value("ExperimentationNoScorecard");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesExperimentationTreatmentAssignmentFailed)
            {
                output = Json::Value("ExperimentationTreatmentAssignmentFailed");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesExperimentationTreatmentAssignmentDisabled)
            {
                output = Json::Value("ExperimentationTreatmentAssignmentDisabled");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesExperimentationInvalidDuration)
            {
                output = Json::Value("ExperimentationInvalidDuration");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesExperimentationMaxExperimentsReached)
            {
                output = Json::Value("ExperimentationMaxExperimentsReached");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesExperimentationExperimentSchedulingInProgress)
            {
                output = Json::Value("ExperimentationExperimentSchedulingInProgress");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesMaxActionDepthExceeded)
            {
                output = Json::Value("MaxActionDepthExceeded");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesTitleNotOnUpdatedPricingPlan)
            {
                output = Json::Value("TitleNotOnUpdatedPricingPlan");
                return;
            }
            if (input == GenericErrorCodes::GenericErrorCodesSnapshotNotFound)
            {
                output = Json::Value("SnapshotNotFound");
                return;
            }
        }
        inline void FromJsonEnum(const Json::Value& input, GenericErrorCodes& output)
        {
            if (!input.isString())
            {
                return;
            }
            const std::string& inputStr = input.asString();
            if (inputStr == "Success")
            {
                output = GenericErrorCodes::GenericErrorCodesSuccess;
                return;
            }
            if (inputStr == "UnkownError")
            {
                output = GenericErrorCodes::GenericErrorCodesUnkownError;
                return;
            }
            if (inputStr == "InvalidParams")
            {
                output = GenericErrorCodes::GenericErrorCodesInvalidParams;
                return;
            }
            if (inputStr == "AccountNotFound")
            {
                output = GenericErrorCodes::GenericErrorCodesAccountNotFound;
                return;
            }
            if (inputStr == "AccountBanned")
            {
                output = GenericErrorCodes::GenericErrorCodesAccountBanned;
                return;
            }
            if (inputStr == "InvalidUsernameOrPassword")
            {
                output = GenericErrorCodes::GenericErrorCodesInvalidUsernameOrPassword;
                return;
            }
            if (inputStr == "InvalidTitleId")
            {
                output = GenericErrorCodes::GenericErrorCodesInvalidTitleId;
                return;
            }
            if (inputStr == "InvalidEmailAddress")
            {
                output = GenericErrorCodes::GenericErrorCodesInvalidEmailAddress;
                return;
            }
            if (inputStr == "EmailAddressNotAvailable")
            {
                output = GenericErrorCodes::GenericErrorCodesEmailAddressNotAvailable;
                return;
            }
            if (inputStr == "InvalidUsername")
            {
                output = GenericErrorCodes::GenericErrorCodesInvalidUsername;
                return;
            }
            if (inputStr == "InvalidPassword")
            {
                output = GenericErrorCodes::GenericErrorCodesInvalidPassword;
                return;
            }
            if (inputStr == "UsernameNotAvailable")
            {
                output = GenericErrorCodes::GenericErrorCodesUsernameNotAvailable;
                return;
            }
            if (inputStr == "InvalidSteamTicket")
            {
                output = GenericErrorCodes::GenericErrorCodesInvalidSteamTicket;
                return;
            }
            if (inputStr == "AccountAlreadyLinked")
            {
                output = GenericErrorCodes::GenericErrorCodesAccountAlreadyLinked;
                return;
            }
            if (inputStr == "LinkedAccountAlreadyClaimed")
            {
                output = GenericErrorCodes::GenericErrorCodesLinkedAccountAlreadyClaimed;
                return;
            }
            if (inputStr == "InvalidFacebookToken")
            {
                output = GenericErrorCodes::GenericErrorCodesInvalidFacebookToken;
                return;
            }
            if (inputStr == "AccountNotLinked")
            {
                output = GenericErrorCodes::GenericErrorCodesAccountNotLinked;
                return;
            }
            if (inputStr == "FailedByPaymentProvider")
            {
                output = GenericErrorCodes::GenericErrorCodesFailedByPaymentProvider;
                return;
            }
            if (inputStr == "CouponCodeNotFound")
            {
                output = GenericErrorCodes::GenericErrorCodesCouponCodeNotFound;
                return;
            }
            if (inputStr == "InvalidContainerItem")
            {
                output = GenericErrorCodes::GenericErrorCodesInvalidContainerItem;
                return;
            }
            if (inputStr == "ContainerNotOwned")
            {
                output = GenericErrorCodes::GenericErrorCodesContainerNotOwned;
                return;
            }
            if (inputStr == "KeyNotOwned")
            {
                output = GenericErrorCodes::GenericErrorCodesKeyNotOwned;
                return;
            }
            if (inputStr == "InvalidItemIdInTable")
            {
                output = GenericErrorCodes::GenericErrorCodesInvalidItemIdInTable;
                return;
            }
            if (inputStr == "InvalidReceipt")
            {
                output = GenericErrorCodes::GenericErrorCodesInvalidReceipt;
                return;
            }
            if (inputStr == "ReceiptAlreadyUsed")
            {
                output = GenericErrorCodes::GenericErrorCodesReceiptAlreadyUsed;
                return;
            }
            if (inputStr == "ReceiptCancelled")
            {
                output = GenericErrorCodes::GenericErrorCodesReceiptCancelled;
                return;
            }
            if (inputStr == "GameNotFound")
            {
                output = GenericErrorCodes::GenericErrorCodesGameNotFound;
                return;
            }
            if (inputStr == "GameModeNotFound")
            {
                output = GenericErrorCodes::GenericErrorCodesGameModeNotFound;
                return;
            }
            if (inputStr == "InvalidGoogleToken")
            {
                output = GenericErrorCodes::GenericErrorCodesInvalidGoogleToken;
                return;
            }
            if (inputStr == "UserIsNotPartOfDeveloper")
            {
                output = GenericErrorCodes::GenericErrorCodesUserIsNotPartOfDeveloper;
                return;
            }
            if (inputStr == "InvalidTitleForDeveloper")
            {
                output = GenericErrorCodes::GenericErrorCodesInvalidTitleForDeveloper;
                return;
            }
            if (inputStr == "TitleNameConflicts")
            {
                output = GenericErrorCodes::GenericErrorCodesTitleNameConflicts;
                return;
            }
            if (inputStr == "UserisNotValid")
            {
                output = GenericErrorCodes::GenericErrorCodesUserisNotValid;
                return;
            }
            if (inputStr == "ValueAlreadyExists")
            {
                output = GenericErrorCodes::GenericErrorCodesValueAlreadyExists;
                return;
            }
            if (inputStr == "BuildNotFound")
            {
                output = GenericErrorCodes::GenericErrorCodesBuildNotFound;
                return;
            }
            if (inputStr == "PlayerNotInGame")
            {
                output = GenericErrorCodes::GenericErrorCodesPlayerNotInGame;
                return;
            }
            if (inputStr == "InvalidTicket")
            {
                output = GenericErrorCodes::GenericErrorCodesInvalidTicket;
                return;
            }
            if (inputStr == "InvalidDeveloper")
            {
                output = GenericErrorCodes::GenericErrorCodesInvalidDeveloper;
                return;
            }
            if (inputStr == "InvalidOrderInfo")
            {
                output = GenericErrorCodes::GenericErrorCodesInvalidOrderInfo;
                return;
            }
            if (inputStr == "RegistrationIncomplete")
            {
                output = GenericErrorCodes::GenericErrorCodesRegistrationIncomplete;
                return;
            }
            if (inputStr == "InvalidPlatform")
            {
                output = GenericErrorCodes::GenericErrorCodesInvalidPlatform;
                return;
            }
            if (inputStr == "UnknownError")
            {
                output = GenericErrorCodes::GenericErrorCodesUnknownError;
                return;
            }
            if (inputStr == "SteamApplicationNotOwned")
            {
                output = GenericErrorCodes::GenericErrorCodesSteamApplicationNotOwned;
                return;
            }
            if (inputStr == "WrongSteamAccount")
            {
                output = GenericErrorCodes::GenericErrorCodesWrongSteamAccount;
                return;
            }
            if (inputStr == "TitleNotActivated")
            {
                output = GenericErrorCodes::GenericErrorCodesTitleNotActivated;
                return;
            }
            if (inputStr == "RegistrationSessionNotFound")
            {
                output = GenericErrorCodes::GenericErrorCodesRegistrationSessionNotFound;
                return;
            }
            if (inputStr == "NoSuchMod")
            {
                output = GenericErrorCodes::GenericErrorCodesNoSuchMod;
                return;
            }
            if (inputStr == "FileNotFound")
            {
                output = GenericErrorCodes::GenericErrorCodesFileNotFound;
                return;
            }
            if (inputStr == "DuplicateEmail")
            {
                output = GenericErrorCodes::GenericErrorCodesDuplicateEmail;
                return;
            }
            if (inputStr == "ItemNotFound")
            {
                output = GenericErrorCodes::GenericErrorCodesItemNotFound;
                return;
            }
            if (inputStr == "ItemNotOwned")
            {
                output = GenericErrorCodes::GenericErrorCodesItemNotOwned;
                return;
            }
            if (inputStr == "ItemNotRecycleable")
            {
                output = GenericErrorCodes::GenericErrorCodesItemNotRecycleable;
                return;
            }
            if (inputStr == "ItemNotAffordable")
            {
                output = GenericErrorCodes::GenericErrorCodesItemNotAffordable;
                return;
            }
            if (inputStr == "InvalidVirtualCurrency")
            {
                output = GenericErrorCodes::GenericErrorCodesInvalidVirtualCurrency;
                return;
            }
            if (inputStr == "WrongVirtualCurrency")
            {
                output = GenericErrorCodes::GenericErrorCodesWrongVirtualCurrency;
                return;
            }
            if (inputStr == "WrongPrice")
            {
                output = GenericErrorCodes::GenericErrorCodesWrongPrice;
                return;
            }
            if (inputStr == "NonPositiveValue")
            {
                output = GenericErrorCodes::GenericErrorCodesNonPositiveValue;
                return;
            }
            if (inputStr == "InvalidRegion")
            {
                output = GenericErrorCodes::GenericErrorCodesInvalidRegion;
                return;
            }
            if (inputStr == "RegionAtCapacity")
            {
                output = GenericErrorCodes::GenericErrorCodesRegionAtCapacity;
                return;
            }
            if (inputStr == "ServerFailedToStart")
            {
                output = GenericErrorCodes::GenericErrorCodesServerFailedToStart;
                return;
            }
            if (inputStr == "NameNotAvailable")
            {
                output = GenericErrorCodes::GenericErrorCodesNameNotAvailable;
                return;
            }
            if (inputStr == "InsufficientFunds")
            {
                output = GenericErrorCodes::GenericErrorCodesInsufficientFunds;
                return;
            }
            if (inputStr == "InvalidDeviceID")
            {
                output = GenericErrorCodes::GenericErrorCodesInvalidDeviceID;
                return;
            }
            if (inputStr == "InvalidPushNotificationToken")
            {
                output = GenericErrorCodes::GenericErrorCodesInvalidPushNotificationToken;
                return;
            }
            if (inputStr == "NoRemainingUses")
            {
                output = GenericErrorCodes::GenericErrorCodesNoRemainingUses;
                return;
            }
            if (inputStr == "InvalidPaymentProvider")
            {
                output = GenericErrorCodes::GenericErrorCodesInvalidPaymentProvider;
                return;
            }
            if (inputStr == "PurchaseInitializationFailure")
            {
                output = GenericErrorCodes::GenericErrorCodesPurchaseInitializationFailure;
                return;
            }
            if (inputStr == "DuplicateUsername")
            {
                output = GenericErrorCodes::GenericErrorCodesDuplicateUsername;
                return;
            }
            if (inputStr == "InvalidBuyerInfo")
            {
                output = GenericErrorCodes::GenericErrorCodesInvalidBuyerInfo;
                return;
            }
            if (inputStr == "NoGameModeParamsSet")
            {
                output = GenericErrorCodes::GenericErrorCodesNoGameModeParamsSet;
                return;
            }
            if (inputStr == "BodyTooLarge")
            {
                output = GenericErrorCodes::GenericErrorCodesBodyTooLarge;
                return;
            }
            if (inputStr == "ReservedWordInBody")
            {
                output = GenericErrorCodes::GenericErrorCodesReservedWordInBody;
                return;
            }
            if (inputStr == "InvalidTypeInBody")
            {
                output = GenericErrorCodes::GenericErrorCodesInvalidTypeInBody;
                return;
            }
            if (inputStr == "InvalidRequest")
            {
                output = GenericErrorCodes::GenericErrorCodesInvalidRequest;
                return;
            }
            if (inputStr == "ReservedEventName")
            {
                output = GenericErrorCodes::GenericErrorCodesReservedEventName;
                return;
            }
            if (inputStr == "InvalidUserStatistics")
            {
                output = GenericErrorCodes::GenericErrorCodesInvalidUserStatistics;
                return;
            }
            if (inputStr == "NotAuthenticated")
            {
                output = GenericErrorCodes::GenericErrorCodesNotAuthenticated;
                return;
            }
            if (inputStr == "StreamAlreadyExists")
            {
                output = GenericErrorCodes::GenericErrorCodesStreamAlreadyExists;
                return;
            }
            if (inputStr == "ErrorCreatingStream")
            {
                output = GenericErrorCodes::GenericErrorCodesErrorCreatingStream;
                return;
            }
            if (inputStr == "StreamNotFound")
            {
                output = GenericErrorCodes::GenericErrorCodesStreamNotFound;
                return;
            }
            if (inputStr == "InvalidAccount")
            {
                output = GenericErrorCodes::GenericErrorCodesInvalidAccount;
                return;
            }
            if (inputStr == "PurchaseDoesNotExist")
            {
                output = GenericErrorCodes::GenericErrorCodesPurchaseDoesNotExist;
                return;
            }
            if (inputStr == "InvalidPurchaseTransactionStatus")
            {
                output = GenericErrorCodes::GenericErrorCodesInvalidPurchaseTransactionStatus;
                return;
            }
            if (inputStr == "APINotEnabledForGameClientAccess")
            {
                output = GenericErrorCodes::GenericErrorCodesAPINotEnabledForGameClientAccess;
                return;
            }
            if (inputStr == "NoPushNotificationARNForTitle")
            {
                output = GenericErrorCodes::GenericErrorCodesNoPushNotificationARNForTitle;
                return;
            }
            if (inputStr == "BuildAlreadyExists")
            {
                output = GenericErrorCodes::GenericErrorCodesBuildAlreadyExists;
                return;
            }
            if (inputStr == "BuildPackageDoesNotExist")
            {
                output = GenericErrorCodes::GenericErrorCodesBuildPackageDoesNotExist;
                return;
            }
            if (inputStr == "CustomAnalyticsEventsNotEnabledForTitle")
            {
                output = GenericErrorCodes::GenericErrorCodesCustomAnalyticsEventsNotEnabledForTitle;
                return;
            }
            if (inputStr == "InvalidSharedGroupId")
            {
                output = GenericErrorCodes::GenericErrorCodesInvalidSharedGroupId;
                return;
            }
            if (inputStr == "NotAuthorized")
            {
                output = GenericErrorCodes::GenericErrorCodesNotAuthorized;
                return;
            }
            if (inputStr == "MissingTitleGoogleProperties")
            {
                output = GenericErrorCodes::GenericErrorCodesMissingTitleGoogleProperties;
                return;
            }
            if (inputStr == "InvalidItemProperties")
            {
                output = GenericErrorCodes::GenericErrorCodesInvalidItemProperties;
                return;
            }
            if (inputStr == "InvalidPSNAuthCode")
            {
                output = GenericErrorCodes::GenericErrorCodesInvalidPSNAuthCode;
                return;
            }
            if (inputStr == "InvalidItemId")
            {
                output = GenericErrorCodes::GenericErrorCodesInvalidItemId;
                return;
            }
            if (inputStr == "PushNotEnabledForAccount")
            {
                output = GenericErrorCodes::GenericErrorCodesPushNotEnabledForAccount;
                return;
            }
            if (inputStr == "PushServiceError")
            {
                output = GenericErrorCodes::GenericErrorCodesPushServiceError;
                return;
            }
            if (inputStr == "ReceiptDoesNotContainInAppItems")
            {
                output = GenericErrorCodes::GenericErrorCodesReceiptDoesNotContainInAppItems;
                return;
            }
            if (inputStr == "ReceiptContainsMultipleInAppItems")
            {
                output = GenericErrorCodes::GenericErrorCodesReceiptContainsMultipleInAppItems;
                return;
            }
            if (inputStr == "InvalidBundleID")
            {
                output = GenericErrorCodes::GenericErrorCodesInvalidBundleID;
                return;
            }
            if (inputStr == "JavascriptException")
            {
                output = GenericErrorCodes::GenericErrorCodesJavascriptException;
                return;
            }
            if (inputStr == "InvalidSessionTicket")
            {
                output = GenericErrorCodes::GenericErrorCodesInvalidSessionTicket;
                return;
            }
            if (inputStr == "UnableToConnectToDatabase")
            {
                output = GenericErrorCodes::GenericErrorCodesUnableToConnectToDatabase;
                return;
            }
            if (inputStr == "InternalServerError")
            {
                output = GenericErrorCodes::GenericErrorCodesInternalServerError;
                return;
            }
            if (inputStr == "InvalidReportDate")
            {
                output = GenericErrorCodes::GenericErrorCodesInvalidReportDate;
                return;
            }
            if (inputStr == "ReportNotAvailable")
            {
                output = GenericErrorCodes::GenericErrorCodesReportNotAvailable;
                return;
            }
            if (inputStr == "DatabaseThroughputExceeded")
            {
                output = GenericErrorCodes::GenericErrorCodesDatabaseThroughputExceeded;
                return;
            }
            if (inputStr == "InvalidGameTicket")
            {
                output = GenericErrorCodes::GenericErrorCodesInvalidGameTicket;
                return;
            }
            if (inputStr == "ExpiredGameTicket")
            {
                output = GenericErrorCodes::GenericErrorCodesExpiredGameTicket;
                return;
            }
            if (inputStr == "GameTicketDoesNotMatchLobby")
            {
                output = GenericErrorCodes::GenericErrorCodesGameTicketDoesNotMatchLobby;
                return;
            }
            if (inputStr == "LinkedDeviceAlreadyClaimed")
            {
                output = GenericErrorCodes::GenericErrorCodesLinkedDeviceAlreadyClaimed;
                return;
            }
            if (inputStr == "DeviceAlreadyLinked")
            {
                output = GenericErrorCodes::GenericErrorCodesDeviceAlreadyLinked;
                return;
            }
            if (inputStr == "DeviceNotLinked")
            {
                output = GenericErrorCodes::GenericErrorCodesDeviceNotLinked;
                return;
            }
            if (inputStr == "PartialFailure")
            {
                output = GenericErrorCodes::GenericErrorCodesPartialFailure;
                return;
            }
            if (inputStr == "PublisherNotSet")
            {
                output = GenericErrorCodes::GenericErrorCodesPublisherNotSet;
                return;
            }
            if (inputStr == "ServiceUnavailable")
            {
                output = GenericErrorCodes::GenericErrorCodesServiceUnavailable;
                return;
            }
            if (inputStr == "VersionNotFound")
            {
                output = GenericErrorCodes::GenericErrorCodesVersionNotFound;
                return;
            }
            if (inputStr == "RevisionNotFound")
            {
                output = GenericErrorCodes::GenericErrorCodesRevisionNotFound;
                return;
            }
            if (inputStr == "InvalidPublisherId")
            {
                output = GenericErrorCodes::GenericErrorCodesInvalidPublisherId;
                return;
            }
            if (inputStr == "DownstreamServiceUnavailable")
            {
                output = GenericErrorCodes::GenericErrorCodesDownstreamServiceUnavailable;
                return;
            }
            if (inputStr == "APINotIncludedInTitleUsageTier")
            {
                output = GenericErrorCodes::GenericErrorCodesAPINotIncludedInTitleUsageTier;
                return;
            }
            if (inputStr == "DAULimitExceeded")
            {
                output = GenericErrorCodes::GenericErrorCodesDAULimitExceeded;
                return;
            }
            if (inputStr == "APIRequestLimitExceeded")
            {
                output = GenericErrorCodes::GenericErrorCodesAPIRequestLimitExceeded;
                return;
            }
            if (inputStr == "InvalidAPIEndpoint")
            {
                output = GenericErrorCodes::GenericErrorCodesInvalidAPIEndpoint;
                return;
            }
            if (inputStr == "BuildNotAvailable")
            {
                output = GenericErrorCodes::GenericErrorCodesBuildNotAvailable;
                return;
            }
            if (inputStr == "ConcurrentEditError")
            {
                output = GenericErrorCodes::GenericErrorCodesConcurrentEditError;
                return;
            }
            if (inputStr == "ContentNotFound")
            {
                output = GenericErrorCodes::GenericErrorCodesContentNotFound;
                return;
            }
            if (inputStr == "CharacterNotFound")
            {
                output = GenericErrorCodes::GenericErrorCodesCharacterNotFound;
                return;
            }
            if (inputStr == "CloudScriptNotFound")
            {
                output = GenericErrorCodes::GenericErrorCodesCloudScriptNotFound;
                return;
            }
            if (inputStr == "ContentQuotaExceeded")
            {
                output = GenericErrorCodes::GenericErrorCodesContentQuotaExceeded;
                return;
            }
            if (inputStr == "InvalidCharacterStatistics")
            {
                output = GenericErrorCodes::GenericErrorCodesInvalidCharacterStatistics;
                return;
            }
            if (inputStr == "PhotonNotEnabledForTitle")
            {
                output = GenericErrorCodes::GenericErrorCodesPhotonNotEnabledForTitle;
                return;
            }
            if (inputStr == "PhotonApplicationNotFound")
            {
                output = GenericErrorCodes::GenericErrorCodesPhotonApplicationNotFound;
                return;
            }
            if (inputStr == "PhotonApplicationNotAssociatedWithTitle")
            {
                output = GenericErrorCodes::GenericErrorCodesPhotonApplicationNotAssociatedWithTitle;
                return;
            }
            if (inputStr == "InvalidEmailOrPassword")
            {
                output = GenericErrorCodes::GenericErrorCodesInvalidEmailOrPassword;
                return;
            }
            if (inputStr == "FacebookAPIError")
            {
                output = GenericErrorCodes::GenericErrorCodesFacebookAPIError;
                return;
            }
            if (inputStr == "InvalidContentType")
            {
                output = GenericErrorCodes::GenericErrorCodesInvalidContentType;
                return;
            }
            if (inputStr == "KeyLengthExceeded")
            {
                output = GenericErrorCodes::GenericErrorCodesKeyLengthExceeded;
                return;
            }
            if (inputStr == "DataLengthExceeded")
            {
                output = GenericErrorCodes::GenericErrorCodesDataLengthExceeded;
                return;
            }
            if (inputStr == "TooManyKeys")
            {
                output = GenericErrorCodes::GenericErrorCodesTooManyKeys;
                return;
            }
            if (inputStr == "FreeTierCannotHaveVirtualCurrency")
            {
                output = GenericErrorCodes::GenericErrorCodesFreeTierCannotHaveVirtualCurrency;
                return;
            }
            if (inputStr == "MissingAmazonSharedKey")
            {
                output = GenericErrorCodes::GenericErrorCodesMissingAmazonSharedKey;
                return;
            }
            if (inputStr == "AmazonValidationError")
            {
                output = GenericErrorCodes::GenericErrorCodesAmazonValidationError;
                return;
            }
            if (inputStr == "InvalidPSNIssuerId")
            {
                output = GenericErrorCodes::GenericErrorCodesInvalidPSNIssuerId;
                return;
            }
            if (inputStr == "PSNInaccessible")
            {
                output = GenericErrorCodes::GenericErrorCodesPSNInaccessible;
                return;
            }
            if (inputStr == "ExpiredAuthToken")
            {
                output = GenericErrorCodes::GenericErrorCodesExpiredAuthToken;
                return;
            }
            if (inputStr == "FailedToGetEntitlements")
            {
                output = GenericErrorCodes::GenericErrorCodesFailedToGetEntitlements;
                return;
            }
            if (inputStr == "FailedToConsumeEntitlement")
            {
                output = GenericErrorCodes::GenericErrorCodesFailedToConsumeEntitlement;
                return;
            }
            if (inputStr == "TradeAcceptingUserNotAllowed")
            {
                output = GenericErrorCodes::GenericErrorCodesTradeAcceptingUserNotAllowed;
                return;
            }
            if (inputStr == "TradeInventoryItemIsAssignedToCharacter")
            {
                output = GenericErrorCodes::GenericErrorCodesTradeInventoryItemIsAssignedToCharacter;
                return;
            }
            if (inputStr == "TradeInventoryItemIsBundle")
            {
                output = GenericErrorCodes::GenericErrorCodesTradeInventoryItemIsBundle;
                return;
            }
            if (inputStr == "TradeStatusNotValidForCancelling")
            {
                output = GenericErrorCodes::GenericErrorCodesTradeStatusNotValidForCancelling;
                return;
            }
            if (inputStr == "TradeStatusNotValidForAccepting")
            {
                output = GenericErrorCodes::GenericErrorCodesTradeStatusNotValidForAccepting;
                return;
            }
            if (inputStr == "TradeDoesNotExist")
            {
                output = GenericErrorCodes::GenericErrorCodesTradeDoesNotExist;
                return;
            }
            if (inputStr == "TradeCancelled")
            {
                output = GenericErrorCodes::GenericErrorCodesTradeCancelled;
                return;
            }
            if (inputStr == "TradeAlreadyFilled")
            {
                output = GenericErrorCodes::GenericErrorCodesTradeAlreadyFilled;
                return;
            }
            if (inputStr == "TradeWaitForStatusTimeout")
            {
                output = GenericErrorCodes::GenericErrorCodesTradeWaitForStatusTimeout;
                return;
            }
            if (inputStr == "TradeInventoryItemExpired")
            {
                output = GenericErrorCodes::GenericErrorCodesTradeInventoryItemExpired;
                return;
            }
            if (inputStr == "TradeMissingOfferedAndAcceptedItems")
            {
                output = GenericErrorCodes::GenericErrorCodesTradeMissingOfferedAndAcceptedItems;
                return;
            }
            if (inputStr == "TradeAcceptedItemIsBundle")
            {
                output = GenericErrorCodes::GenericErrorCodesTradeAcceptedItemIsBundle;
                return;
            }
            if (inputStr == "TradeAcceptedItemIsStackable")
            {
                output = GenericErrorCodes::GenericErrorCodesTradeAcceptedItemIsStackable;
                return;
            }
            if (inputStr == "TradeInventoryItemInvalidStatus")
            {
                output = GenericErrorCodes::GenericErrorCodesTradeInventoryItemInvalidStatus;
                return;
            }
            if (inputStr == "TradeAcceptedCatalogItemInvalid")
            {
                output = GenericErrorCodes::GenericErrorCodesTradeAcceptedCatalogItemInvalid;
                return;
            }
            if (inputStr == "TradeAllowedUsersInvalid")
            {
                output = GenericErrorCodes::GenericErrorCodesTradeAllowedUsersInvalid;
                return;
            }
            if (inputStr == "TradeInventoryItemDoesNotExist")
            {
                output = GenericErrorCodes::GenericErrorCodesTradeInventoryItemDoesNotExist;
                return;
            }
            if (inputStr == "TradeInventoryItemIsConsumed")
            {
                output = GenericErrorCodes::GenericErrorCodesTradeInventoryItemIsConsumed;
                return;
            }
            if (inputStr == "TradeInventoryItemIsStackable")
            {
                output = GenericErrorCodes::GenericErrorCodesTradeInventoryItemIsStackable;
                return;
            }
            if (inputStr == "TradeAcceptedItemsMismatch")
            {
                output = GenericErrorCodes::GenericErrorCodesTradeAcceptedItemsMismatch;
                return;
            }
            if (inputStr == "InvalidKongregateToken")
            {
                output = GenericErrorCodes::GenericErrorCodesInvalidKongregateToken;
                return;
            }
            if (inputStr == "FeatureNotConfiguredForTitle")
            {
                output = GenericErrorCodes::GenericErrorCodesFeatureNotConfiguredForTitle;
                return;
            }
            if (inputStr == "NoMatchingCatalogItemForReceipt")
            {
                output = GenericErrorCodes::GenericErrorCodesNoMatchingCatalogItemForReceipt;
                return;
            }
            if (inputStr == "InvalidCurrencyCode")
            {
                output = GenericErrorCodes::GenericErrorCodesInvalidCurrencyCode;
                return;
            }
            if (inputStr == "NoRealMoneyPriceForCatalogItem")
            {
                output = GenericErrorCodes::GenericErrorCodesNoRealMoneyPriceForCatalogItem;
                return;
            }
            if (inputStr == "TradeInventoryItemIsNotTradable")
            {
                output = GenericErrorCodes::GenericErrorCodesTradeInventoryItemIsNotTradable;
                return;
            }
            if (inputStr == "TradeAcceptedCatalogItemIsNotTradable")
            {
                output = GenericErrorCodes::GenericErrorCodesTradeAcceptedCatalogItemIsNotTradable;
                return;
            }
            if (inputStr == "UsersAlreadyFriends")
            {
                output = GenericErrorCodes::GenericErrorCodesUsersAlreadyFriends;
                return;
            }
            if (inputStr == "LinkedIdentifierAlreadyClaimed")
            {
                output = GenericErrorCodes::GenericErrorCodesLinkedIdentifierAlreadyClaimed;
                return;
            }
            if (inputStr == "CustomIdNotLinked")
            {
                output = GenericErrorCodes::GenericErrorCodesCustomIdNotLinked;
                return;
            }
            if (inputStr == "TotalDataSizeExceeded")
            {
                output = GenericErrorCodes::GenericErrorCodesTotalDataSizeExceeded;
                return;
            }
            if (inputStr == "DeleteKeyConflict")
            {
                output = GenericErrorCodes::GenericErrorCodesDeleteKeyConflict;
                return;
            }
            if (inputStr == "InvalidXboxLiveToken")
            {
                output = GenericErrorCodes::GenericErrorCodesInvalidXboxLiveToken;
                return;
            }
            if (inputStr == "ExpiredXboxLiveToken")
            {
                output = GenericErrorCodes::GenericErrorCodesExpiredXboxLiveToken;
                return;
            }
            if (inputStr == "ResettableStatisticVersionRequired")
            {
                output = GenericErrorCodes::GenericErrorCodesResettableStatisticVersionRequired;
                return;
            }
            if (inputStr == "NotAuthorizedByTitle")
            {
                output = GenericErrorCodes::GenericErrorCodesNotAuthorizedByTitle;
                return;
            }
            if (inputStr == "NoPartnerEnabled")
            {
                output = GenericErrorCodes::GenericErrorCodesNoPartnerEnabled;
                return;
            }
            if (inputStr == "InvalidPartnerResponse")
            {
                output = GenericErrorCodes::GenericErrorCodesInvalidPartnerResponse;
                return;
            }
            if (inputStr == "APINotEnabledForGameServerAccess")
            {
                output = GenericErrorCodes::GenericErrorCodesAPINotEnabledForGameServerAccess;
                return;
            }
            if (inputStr == "StatisticNotFound")
            {
                output = GenericErrorCodes::GenericErrorCodesStatisticNotFound;
                return;
            }
            if (inputStr == "StatisticNameConflict")
            {
                output = GenericErrorCodes::GenericErrorCodesStatisticNameConflict;
                return;
            }
            if (inputStr == "StatisticVersionClosedForWrites")
            {
                output = GenericErrorCodes::GenericErrorCodesStatisticVersionClosedForWrites;
                return;
            }
            if (inputStr == "StatisticVersionInvalid")
            {
                output = GenericErrorCodes::GenericErrorCodesStatisticVersionInvalid;
                return;
            }
            if (inputStr == "APIClientRequestRateLimitExceeded")
            {
                output = GenericErrorCodes::GenericErrorCodesAPIClientRequestRateLimitExceeded;
                return;
            }
            if (inputStr == "InvalidJSONContent")
            {
                output = GenericErrorCodes::GenericErrorCodesInvalidJSONContent;
                return;
            }
            if (inputStr == "InvalidDropTable")
            {
                output = GenericErrorCodes::GenericErrorCodesInvalidDropTable;
                return;
            }
            if (inputStr == "StatisticVersionAlreadyIncrementedForScheduledInterval")
            {
                output = GenericErrorCodes::GenericErrorCodesStatisticVersionAlreadyIncrementedForScheduledInterval;
                return;
            }
            if (inputStr == "StatisticCountLimitExceeded")
            {
                output = GenericErrorCodes::GenericErrorCodesStatisticCountLimitExceeded;
                return;
            }
            if (inputStr == "StatisticVersionIncrementRateExceeded")
            {
                output = GenericErrorCodes::GenericErrorCodesStatisticVersionIncrementRateExceeded;
                return;
            }
            if (inputStr == "ContainerKeyInvalid")
            {
                output = GenericErrorCodes::GenericErrorCodesContainerKeyInvalid;
                return;
            }
            if (inputStr == "CloudScriptExecutionTimeLimitExceeded")
            {
                output = GenericErrorCodes::GenericErrorCodesCloudScriptExecutionTimeLimitExceeded;
                return;
            }
            if (inputStr == "NoWritePermissionsForEvent")
            {
                output = GenericErrorCodes::GenericErrorCodesNoWritePermissionsForEvent;
                return;
            }
            if (inputStr == "CloudScriptFunctionArgumentSizeExceeded")
            {
                output = GenericErrorCodes::GenericErrorCodesCloudScriptFunctionArgumentSizeExceeded;
                return;
            }
            if (inputStr == "CloudScriptAPIRequestCountExceeded")
            {
                output = GenericErrorCodes::GenericErrorCodesCloudScriptAPIRequestCountExceeded;
                return;
            }
            if (inputStr == "CloudScriptAPIRequestError")
            {
                output = GenericErrorCodes::GenericErrorCodesCloudScriptAPIRequestError;
                return;
            }
            if (inputStr == "CloudScriptHTTPRequestError")
            {
                output = GenericErrorCodes::GenericErrorCodesCloudScriptHTTPRequestError;
                return;
            }
            if (inputStr == "InsufficientGuildRole")
            {
                output = GenericErrorCodes::GenericErrorCodesInsufficientGuildRole;
                return;
            }
            if (inputStr == "GuildNotFound")
            {
                output = GenericErrorCodes::GenericErrorCodesGuildNotFound;
                return;
            }
            if (inputStr == "OverLimit")
            {
                output = GenericErrorCodes::GenericErrorCodesOverLimit;
                return;
            }
            if (inputStr == "EventNotFound")
            {
                output = GenericErrorCodes::GenericErrorCodesEventNotFound;
                return;
            }
            if (inputStr == "InvalidEventField")
            {
                output = GenericErrorCodes::GenericErrorCodesInvalidEventField;
                return;
            }
            if (inputStr == "InvalidEventName")
            {
                output = GenericErrorCodes::GenericErrorCodesInvalidEventName;
                return;
            }
            if (inputStr == "CatalogNotConfigured")
            {
                output = GenericErrorCodes::GenericErrorCodesCatalogNotConfigured;
                return;
            }
            if (inputStr == "OperationNotSupportedForPlatform")
            {
                output = GenericErrorCodes::GenericErrorCodesOperationNotSupportedForPlatform;
                return;
            }
            if (inputStr == "SegmentNotFound")
            {
                output = GenericErrorCodes::GenericErrorCodesSegmentNotFound;
                return;
            }
            if (inputStr == "StoreNotFound")
            {
                output = GenericErrorCodes::GenericErrorCodesStoreNotFound;
                return;
            }
            if (inputStr == "InvalidStatisticName")
            {
                output = GenericErrorCodes::GenericErrorCodesInvalidStatisticName;
                return;
            }
            if (inputStr == "TitleNotQualifiedForLimit")
            {
                output = GenericErrorCodes::GenericErrorCodesTitleNotQualifiedForLimit;
                return;
            }
            if (inputStr == "InvalidServiceLimitLevel")
            {
                output = GenericErrorCodes::GenericErrorCodesInvalidServiceLimitLevel;
                return;
            }
            if (inputStr == "ServiceLimitLevelInTransition")
            {
                output = GenericErrorCodes::GenericErrorCodesServiceLimitLevelInTransition;
                return;
            }
            if (inputStr == "CouponAlreadyRedeemed")
            {
                output = GenericErrorCodes::GenericErrorCodesCouponAlreadyRedeemed;
                return;
            }
            if (inputStr == "GameServerBuildSizeLimitExceeded")
            {
                output = GenericErrorCodes::GenericErrorCodesGameServerBuildSizeLimitExceeded;
                return;
            }
            if (inputStr == "GameServerBuildCountLimitExceeded")
            {
                output = GenericErrorCodes::GenericErrorCodesGameServerBuildCountLimitExceeded;
                return;
            }
            if (inputStr == "VirtualCurrencyCountLimitExceeded")
            {
                output = GenericErrorCodes::GenericErrorCodesVirtualCurrencyCountLimitExceeded;
                return;
            }
            if (inputStr == "VirtualCurrencyCodeExists")
            {
                output = GenericErrorCodes::GenericErrorCodesVirtualCurrencyCodeExists;
                return;
            }
            if (inputStr == "TitleNewsItemCountLimitExceeded")
            {
                output = GenericErrorCodes::GenericErrorCodesTitleNewsItemCountLimitExceeded;
                return;
            }
            if (inputStr == "InvalidTwitchToken")
            {
                output = GenericErrorCodes::GenericErrorCodesInvalidTwitchToken;
                return;
            }
            if (inputStr == "TwitchResponseError")
            {
                output = GenericErrorCodes::GenericErrorCodesTwitchResponseError;
                return;
            }
            if (inputStr == "ProfaneDisplayName")
            {
                output = GenericErrorCodes::GenericErrorCodesProfaneDisplayName;
                return;
            }
            if (inputStr == "UserAlreadyAdded")
            {
                output = GenericErrorCodes::GenericErrorCodesUserAlreadyAdded;
                return;
            }
            if (inputStr == "InvalidVirtualCurrencyCode")
            {
                output = GenericErrorCodes::GenericErrorCodesInvalidVirtualCurrencyCode;
                return;
            }
            if (inputStr == "VirtualCurrencyCannotBeDeleted")
            {
                output = GenericErrorCodes::GenericErrorCodesVirtualCurrencyCannotBeDeleted;
                return;
            }
            if (inputStr == "IdentifierAlreadyClaimed")
            {
                output = GenericErrorCodes::GenericErrorCodesIdentifierAlreadyClaimed;
                return;
            }
            if (inputStr == "IdentifierNotLinked")
            {
                output = GenericErrorCodes::GenericErrorCodesIdentifierNotLinked;
                return;
            }
            if (inputStr == "InvalidContinuationToken")
            {
                output = GenericErrorCodes::GenericErrorCodesInvalidContinuationToken;
                return;
            }
            if (inputStr == "ExpiredContinuationToken")
            {
                output = GenericErrorCodes::GenericErrorCodesExpiredContinuationToken;
                return;
            }
            if (inputStr == "InvalidSegment")
            {
                output = GenericErrorCodes::GenericErrorCodesInvalidSegment;
                return;
            }
            if (inputStr == "InvalidSessionId")
            {
                output = GenericErrorCodes::GenericErrorCodesInvalidSessionId;
                return;
            }
            if (inputStr == "SessionLogNotFound")
            {
                output = GenericErrorCodes::GenericErrorCodesSessionLogNotFound;
                return;
            }
            if (inputStr == "InvalidSearchTerm")
            {
                output = GenericErrorCodes::GenericErrorCodesInvalidSearchTerm;
                return;
            }
            if (inputStr == "TwoFactorAuthenticationTokenRequired")
            {
                output = GenericErrorCodes::GenericErrorCodesTwoFactorAuthenticationTokenRequired;
                return;
            }
            if (inputStr == "GameServerHostCountLimitExceeded")
            {
                output = GenericErrorCodes::GenericErrorCodesGameServerHostCountLimitExceeded;
                return;
            }
            if (inputStr == "PlayerTagCountLimitExceeded")
            {
                output = GenericErrorCodes::GenericErrorCodesPlayerTagCountLimitExceeded;
                return;
            }
            if (inputStr == "RequestAlreadyRunning")
            {
                output = GenericErrorCodes::GenericErrorCodesRequestAlreadyRunning;
                return;
            }
            if (inputStr == "ActionGroupNotFound")
            {
                output = GenericErrorCodes::GenericErrorCodesActionGroupNotFound;
                return;
            }
            if (inputStr == "MaximumSegmentBulkActionJobsRunning")
            {
                output = GenericErrorCodes::GenericErrorCodesMaximumSegmentBulkActionJobsRunning;
                return;
            }
            if (inputStr == "NoActionsOnPlayersInSegmentJob")
            {
                output = GenericErrorCodes::GenericErrorCodesNoActionsOnPlayersInSegmentJob;
                return;
            }
            if (inputStr == "DuplicateStatisticName")
            {
                output = GenericErrorCodes::GenericErrorCodesDuplicateStatisticName;
                return;
            }
            if (inputStr == "ScheduledTaskNameConflict")
            {
                output = GenericErrorCodes::GenericErrorCodesScheduledTaskNameConflict;
                return;
            }
            if (inputStr == "ScheduledTaskCreateConflict")
            {
                output = GenericErrorCodes::GenericErrorCodesScheduledTaskCreateConflict;
                return;
            }
            if (inputStr == "InvalidScheduledTaskName")
            {
                output = GenericErrorCodes::GenericErrorCodesInvalidScheduledTaskName;
                return;
            }
            if (inputStr == "InvalidTaskSchedule")
            {
                output = GenericErrorCodes::GenericErrorCodesInvalidTaskSchedule;
                return;
            }
            if (inputStr == "SteamNotEnabledForTitle")
            {
                output = GenericErrorCodes::GenericErrorCodesSteamNotEnabledForTitle;
                return;
            }
            if (inputStr == "LimitNotAnUpgradeOption")
            {
                output = GenericErrorCodes::GenericErrorCodesLimitNotAnUpgradeOption;
                return;
            }
            if (inputStr == "NoSecretKeyEnabledForCloudScript")
            {
                output = GenericErrorCodes::GenericErrorCodesNoSecretKeyEnabledForCloudScript;
                return;
            }
            if (inputStr == "TaskNotFound")
            {
                output = GenericErrorCodes::GenericErrorCodesTaskNotFound;
                return;
            }
            if (inputStr == "TaskInstanceNotFound")
            {
                output = GenericErrorCodes::GenericErrorCodesTaskInstanceNotFound;
                return;
            }
            if (inputStr == "InvalidIdentityProviderId")
            {
                output = GenericErrorCodes::GenericErrorCodesInvalidIdentityProviderId;
                return;
            }
            if (inputStr == "MisconfiguredIdentityProvider")
            {
                output = GenericErrorCodes::GenericErrorCodesMisconfiguredIdentityProvider;
                return;
            }
            if (inputStr == "InvalidScheduledTaskType")
            {
                output = GenericErrorCodes::GenericErrorCodesInvalidScheduledTaskType;
                return;
            }
            if (inputStr == "BillingInformationRequired")
            {
                output = GenericErrorCodes::GenericErrorCodesBillingInformationRequired;
                return;
            }
            if (inputStr == "LimitedEditionItemUnavailable")
            {
                output = GenericErrorCodes::GenericErrorCodesLimitedEditionItemUnavailable;
                return;
            }
            if (inputStr == "InvalidAdPlacementAndReward")
            {
                output = GenericErrorCodes::GenericErrorCodesInvalidAdPlacementAndReward;
                return;
            }
            if (inputStr == "AllAdPlacementViewsAlreadyConsumed")
            {
                output = GenericErrorCodes::GenericErrorCodesAllAdPlacementViewsAlreadyConsumed;
                return;
            }
            if (inputStr == "GoogleOAuthNotConfiguredForTitle")
            {
                output = GenericErrorCodes::GenericErrorCodesGoogleOAuthNotConfiguredForTitle;
                return;
            }
            if (inputStr == "GoogleOAuthError")
            {
                output = GenericErrorCodes::GenericErrorCodesGoogleOAuthError;
                return;
            }
            if (inputStr == "UserNotFriend")
            {
                output = GenericErrorCodes::GenericErrorCodesUserNotFriend;
                return;
            }
            if (inputStr == "InvalidSignature")
            {
                output = GenericErrorCodes::GenericErrorCodesInvalidSignature;
                return;
            }
            if (inputStr == "InvalidPublicKey")
            {
                output = GenericErrorCodes::GenericErrorCodesInvalidPublicKey;
                return;
            }
            if (inputStr == "GoogleOAuthNoIdTokenIncludedInResponse")
            {
                output = GenericErrorCodes::GenericErrorCodesGoogleOAuthNoIdTokenIncludedInResponse;
                return;
            }
            if (inputStr == "StatisticUpdateInProgress")
            {
                output = GenericErrorCodes::GenericErrorCodesStatisticUpdateInProgress;
                return;
            }
            if (inputStr == "LeaderboardVersionNotAvailable")
            {
                output = GenericErrorCodes::GenericErrorCodesLeaderboardVersionNotAvailable;
                return;
            }
            if (inputStr == "StatisticAlreadyHasPrizeTable")
            {
                output = GenericErrorCodes::GenericErrorCodesStatisticAlreadyHasPrizeTable;
                return;
            }
            if (inputStr == "PrizeTableHasOverlappingRanks")
            {
                output = GenericErrorCodes::GenericErrorCodesPrizeTableHasOverlappingRanks;
                return;
            }
            if (inputStr == "PrizeTableHasMissingRanks")
            {
                output = GenericErrorCodes::GenericErrorCodesPrizeTableHasMissingRanks;
                return;
            }
            if (inputStr == "PrizeTableRankStartsAtZero")
            {
                output = GenericErrorCodes::GenericErrorCodesPrizeTableRankStartsAtZero;
                return;
            }
            if (inputStr == "InvalidStatistic")
            {
                output = GenericErrorCodes::GenericErrorCodesInvalidStatistic;
                return;
            }
            if (inputStr == "ExpressionParseFailure")
            {
                output = GenericErrorCodes::GenericErrorCodesExpressionParseFailure;
                return;
            }
            if (inputStr == "ExpressionInvokeFailure")
            {
                output = GenericErrorCodes::GenericErrorCodesExpressionInvokeFailure;
                return;
            }
            if (inputStr == "ExpressionTooLong")
            {
                output = GenericErrorCodes::GenericErrorCodesExpressionTooLong;
                return;
            }
            if (inputStr == "DataUpdateRateExceeded")
            {
                output = GenericErrorCodes::GenericErrorCodesDataUpdateRateExceeded;
                return;
            }
            if (inputStr == "RestrictedEmailDomain")
            {
                output = GenericErrorCodes::GenericErrorCodesRestrictedEmailDomain;
                return;
            }
            if (inputStr == "EncryptionKeyDisabled")
            {
                output = GenericErrorCodes::GenericErrorCodesEncryptionKeyDisabled;
                return;
            }
            if (inputStr == "EncryptionKeyMissing")
            {
                output = GenericErrorCodes::GenericErrorCodesEncryptionKeyMissing;
                return;
            }
            if (inputStr == "EncryptionKeyBroken")
            {
                output = GenericErrorCodes::GenericErrorCodesEncryptionKeyBroken;
                return;
            }
            if (inputStr == "NoSharedSecretKeyConfigured")
            {
                output = GenericErrorCodes::GenericErrorCodesNoSharedSecretKeyConfigured;
                return;
            }
            if (inputStr == "SecretKeyNotFound")
            {
                output = GenericErrorCodes::GenericErrorCodesSecretKeyNotFound;
                return;
            }
            if (inputStr == "PlayerSecretAlreadyConfigured")
            {
                output = GenericErrorCodes::GenericErrorCodesPlayerSecretAlreadyConfigured;
                return;
            }
            if (inputStr == "APIRequestsDisabledForTitle")
            {
                output = GenericErrorCodes::GenericErrorCodesAPIRequestsDisabledForTitle;
                return;
            }
            if (inputStr == "InvalidSharedSecretKey")
            {
                output = GenericErrorCodes::GenericErrorCodesInvalidSharedSecretKey;
                return;
            }
            if (inputStr == "PrizeTableHasNoRanks")
            {
                output = GenericErrorCodes::GenericErrorCodesPrizeTableHasNoRanks;
                return;
            }
            if (inputStr == "ProfileDoesNotExist")
            {
                output = GenericErrorCodes::GenericErrorCodesProfileDoesNotExist;
                return;
            }
            if (inputStr == "ContentS3OriginBucketNotConfigured")
            {
                output = GenericErrorCodes::GenericErrorCodesContentS3OriginBucketNotConfigured;
                return;
            }
            if (inputStr == "InvalidEnvironmentForReceipt")
            {
                output = GenericErrorCodes::GenericErrorCodesInvalidEnvironmentForReceipt;
                return;
            }
            if (inputStr == "EncryptedRequestNotAllowed")
            {
                output = GenericErrorCodes::GenericErrorCodesEncryptedRequestNotAllowed;
                return;
            }
            if (inputStr == "SignedRequestNotAllowed")
            {
                output = GenericErrorCodes::GenericErrorCodesSignedRequestNotAllowed;
                return;
            }
            if (inputStr == "RequestViewConstraintParamsNotAllowed")
            {
                output = GenericErrorCodes::GenericErrorCodesRequestViewConstraintParamsNotAllowed;
                return;
            }
            if (inputStr == "BadPartnerConfiguration")
            {
                output = GenericErrorCodes::GenericErrorCodesBadPartnerConfiguration;
                return;
            }
            if (inputStr == "XboxBPCertificateFailure")
            {
                output = GenericErrorCodes::GenericErrorCodesXboxBPCertificateFailure;
                return;
            }
            if (inputStr == "XboxXASSExchangeFailure")
            {
                output = GenericErrorCodes::GenericErrorCodesXboxXASSExchangeFailure;
                return;
            }
            if (inputStr == "InvalidEntityId")
            {
                output = GenericErrorCodes::GenericErrorCodesInvalidEntityId;
                return;
            }
            if (inputStr == "StatisticValueAggregationOverflow")
            {
                output = GenericErrorCodes::GenericErrorCodesStatisticValueAggregationOverflow;
                return;
            }
            if (inputStr == "EmailMessageFromAddressIsMissing")
            {
                output = GenericErrorCodes::GenericErrorCodesEmailMessageFromAddressIsMissing;
                return;
            }
            if (inputStr == "EmailMessageToAddressIsMissing")
            {
                output = GenericErrorCodes::GenericErrorCodesEmailMessageToAddressIsMissing;
                return;
            }
            if (inputStr == "SmtpServerAuthenticationError")
            {
                output = GenericErrorCodes::GenericErrorCodesSmtpServerAuthenticationError;
                return;
            }
            if (inputStr == "SmtpServerLimitExceeded")
            {
                output = GenericErrorCodes::GenericErrorCodesSmtpServerLimitExceeded;
                return;
            }
            if (inputStr == "SmtpServerInsufficientStorage")
            {
                output = GenericErrorCodes::GenericErrorCodesSmtpServerInsufficientStorage;
                return;
            }
            if (inputStr == "SmtpServerCommunicationError")
            {
                output = GenericErrorCodes::GenericErrorCodesSmtpServerCommunicationError;
                return;
            }
            if (inputStr == "SmtpServerGeneralFailure")
            {
                output = GenericErrorCodes::GenericErrorCodesSmtpServerGeneralFailure;
                return;
            }
            if (inputStr == "EmailClientTimeout")
            {
                output = GenericErrorCodes::GenericErrorCodesEmailClientTimeout;
                return;
            }
            if (inputStr == "EmailClientCanceledTask")
            {
                output = GenericErrorCodes::GenericErrorCodesEmailClientCanceledTask;
                return;
            }
            if (inputStr == "EmailTemplateMissing")
            {
                output = GenericErrorCodes::GenericErrorCodesEmailTemplateMissing;
                return;
            }
            if (inputStr == "InvalidHostForTitleId")
            {
                output = GenericErrorCodes::GenericErrorCodesInvalidHostForTitleId;
                return;
            }
            if (inputStr == "EmailConfirmationTokenDoesNotExist")
            {
                output = GenericErrorCodes::GenericErrorCodesEmailConfirmationTokenDoesNotExist;
                return;
            }
            if (inputStr == "EmailConfirmationTokenExpired")
            {
                output = GenericErrorCodes::GenericErrorCodesEmailConfirmationTokenExpired;
                return;
            }
            if (inputStr == "AccountDeleted")
            {
                output = GenericErrorCodes::GenericErrorCodesAccountDeleted;
                return;
            }
            if (inputStr == "PlayerSecretNotConfigured")
            {
                output = GenericErrorCodes::GenericErrorCodesPlayerSecretNotConfigured;
                return;
            }
            if (inputStr == "InvalidSignatureTime")
            {
                output = GenericErrorCodes::GenericErrorCodesInvalidSignatureTime;
                return;
            }
            if (inputStr == "NoContactEmailAddressFound")
            {
                output = GenericErrorCodes::GenericErrorCodesNoContactEmailAddressFound;
                return;
            }
            if (inputStr == "InvalidAuthToken")
            {
                output = GenericErrorCodes::GenericErrorCodesInvalidAuthToken;
                return;
            }
            if (inputStr == "AuthTokenDoesNotExist")
            {
                output = GenericErrorCodes::GenericErrorCodesAuthTokenDoesNotExist;
                return;
            }
            if (inputStr == "AuthTokenExpired")
            {
                output = GenericErrorCodes::GenericErrorCodesAuthTokenExpired;
                return;
            }
            if (inputStr == "AuthTokenAlreadyUsedToResetPassword")
            {
                output = GenericErrorCodes::GenericErrorCodesAuthTokenAlreadyUsedToResetPassword;
                return;
            }
            if (inputStr == "MembershipNameTooLong")
            {
                output = GenericErrorCodes::GenericErrorCodesMembershipNameTooLong;
                return;
            }
            if (inputStr == "MembershipNotFound")
            {
                output = GenericErrorCodes::GenericErrorCodesMembershipNotFound;
                return;
            }
            if (inputStr == "GoogleServiceAccountInvalid")
            {
                output = GenericErrorCodes::GenericErrorCodesGoogleServiceAccountInvalid;
                return;
            }
            if (inputStr == "GoogleServiceAccountParseFailure")
            {
                output = GenericErrorCodes::GenericErrorCodesGoogleServiceAccountParseFailure;
                return;
            }
            if (inputStr == "EntityTokenMissing")
            {
                output = GenericErrorCodes::GenericErrorCodesEntityTokenMissing;
                return;
            }
            if (inputStr == "EntityTokenInvalid")
            {
                output = GenericErrorCodes::GenericErrorCodesEntityTokenInvalid;
                return;
            }
            if (inputStr == "EntityTokenExpired")
            {
                output = GenericErrorCodes::GenericErrorCodesEntityTokenExpired;
                return;
            }
            if (inputStr == "EntityTokenRevoked")
            {
                output = GenericErrorCodes::GenericErrorCodesEntityTokenRevoked;
                return;
            }
            if (inputStr == "InvalidProductForSubscription")
            {
                output = GenericErrorCodes::GenericErrorCodesInvalidProductForSubscription;
                return;
            }
            if (inputStr == "XboxInaccessible")
            {
                output = GenericErrorCodes::GenericErrorCodesXboxInaccessible;
                return;
            }
            if (inputStr == "SubscriptionAlreadyTaken")
            {
                output = GenericErrorCodes::GenericErrorCodesSubscriptionAlreadyTaken;
                return;
            }
            if (inputStr == "SmtpAddonNotEnabled")
            {
                output = GenericErrorCodes::GenericErrorCodesSmtpAddonNotEnabled;
                return;
            }
            if (inputStr == "APIConcurrentRequestLimitExceeded")
            {
                output = GenericErrorCodes::GenericErrorCodesAPIConcurrentRequestLimitExceeded;
                return;
            }
            if (inputStr == "XboxRejectedXSTSExchangeRequest")
            {
                output = GenericErrorCodes::GenericErrorCodesXboxRejectedXSTSExchangeRequest;
                return;
            }
            if (inputStr == "VariableNotDefined")
            {
                output = GenericErrorCodes::GenericErrorCodesVariableNotDefined;
                return;
            }
            if (inputStr == "TemplateVersionNotDefined")
            {
                output = GenericErrorCodes::GenericErrorCodesTemplateVersionNotDefined;
                return;
            }
            if (inputStr == "FileTooLarge")
            {
                output = GenericErrorCodes::GenericErrorCodesFileTooLarge;
                return;
            }
            if (inputStr == "TitleDeleted")
            {
                output = GenericErrorCodes::GenericErrorCodesTitleDeleted;
                return;
            }
            if (inputStr == "TitleContainsUserAccounts")
            {
                output = GenericErrorCodes::GenericErrorCodesTitleContainsUserAccounts;
                return;
            }
            if (inputStr == "TitleDeletionPlayerCleanupFailure")
            {
                output = GenericErrorCodes::GenericErrorCodesTitleDeletionPlayerCleanupFailure;
                return;
            }
            if (inputStr == "EntityFileOperationPending")
            {
                output = GenericErrorCodes::GenericErrorCodesEntityFileOperationPending;
                return;
            }
            if (inputStr == "NoEntityFileOperationPending")
            {
                output = GenericErrorCodes::GenericErrorCodesNoEntityFileOperationPending;
                return;
            }
            if (inputStr == "EntityProfileVersionMismatch")
            {
                output = GenericErrorCodes::GenericErrorCodesEntityProfileVersionMismatch;
                return;
            }
            if (inputStr == "TemplateVersionTooOld")
            {
                output = GenericErrorCodes::GenericErrorCodesTemplateVersionTooOld;
                return;
            }
            if (inputStr == "MembershipDefinitionInUse")
            {
                output = GenericErrorCodes::GenericErrorCodesMembershipDefinitionInUse;
                return;
            }
            if (inputStr == "PaymentPageNotConfigured")
            {
                output = GenericErrorCodes::GenericErrorCodesPaymentPageNotConfigured;
                return;
            }
            if (inputStr == "FailedLoginAttemptRateLimitExceeded")
            {
                output = GenericErrorCodes::GenericErrorCodesFailedLoginAttemptRateLimitExceeded;
                return;
            }
            if (inputStr == "EntityBlockedByGroup")
            {
                output = GenericErrorCodes::GenericErrorCodesEntityBlockedByGroup;
                return;
            }
            if (inputStr == "RoleDoesNotExist")
            {
                output = GenericErrorCodes::GenericErrorCodesRoleDoesNotExist;
                return;
            }
            if (inputStr == "EntityIsAlreadyMember")
            {
                output = GenericErrorCodes::GenericErrorCodesEntityIsAlreadyMember;
                return;
            }
            if (inputStr == "DuplicateRoleId")
            {
                output = GenericErrorCodes::GenericErrorCodesDuplicateRoleId;
                return;
            }
            if (inputStr == "GroupInvitationNotFound")
            {
                output = GenericErrorCodes::GenericErrorCodesGroupInvitationNotFound;
                return;
            }
            if (inputStr == "GroupApplicationNotFound")
            {
                output = GenericErrorCodes::GenericErrorCodesGroupApplicationNotFound;
                return;
            }
            if (inputStr == "OutstandingInvitationAcceptedInstead")
            {
                output = GenericErrorCodes::GenericErrorCodesOutstandingInvitationAcceptedInstead;
                return;
            }
            if (inputStr == "OutstandingApplicationAcceptedInstead")
            {
                output = GenericErrorCodes::GenericErrorCodesOutstandingApplicationAcceptedInstead;
                return;
            }
            if (inputStr == "RoleIsGroupDefaultMember")
            {
                output = GenericErrorCodes::GenericErrorCodesRoleIsGroupDefaultMember;
                return;
            }
            if (inputStr == "RoleIsGroupAdmin")
            {
                output = GenericErrorCodes::GenericErrorCodesRoleIsGroupAdmin;
                return;
            }
            if (inputStr == "RoleNameNotAvailable")
            {
                output = GenericErrorCodes::GenericErrorCodesRoleNameNotAvailable;
                return;
            }
            if (inputStr == "GroupNameNotAvailable")
            {
                output = GenericErrorCodes::GenericErrorCodesGroupNameNotAvailable;
                return;
            }
            if (inputStr == "EmailReportAlreadySent")
            {
                output = GenericErrorCodes::GenericErrorCodesEmailReportAlreadySent;
                return;
            }
            if (inputStr == "EmailReportRecipientBlacklisted")
            {
                output = GenericErrorCodes::GenericErrorCodesEmailReportRecipientBlacklisted;
                return;
            }
            if (inputStr == "EventNamespaceNotAllowed")
            {
                output = GenericErrorCodes::GenericErrorCodesEventNamespaceNotAllowed;
                return;
            }
            if (inputStr == "EventEntityNotAllowed")
            {
                output = GenericErrorCodes::GenericErrorCodesEventEntityNotAllowed;
                return;
            }
            if (inputStr == "InvalidEntityType")
            {
                output = GenericErrorCodes::GenericErrorCodesInvalidEntityType;
                return;
            }
            if (inputStr == "NullTokenResultFromAad")
            {
                output = GenericErrorCodes::GenericErrorCodesNullTokenResultFromAad;
                return;
            }
            if (inputStr == "InvalidTokenResultFromAad")
            {
                output = GenericErrorCodes::GenericErrorCodesInvalidTokenResultFromAad;
                return;
            }
            if (inputStr == "NoValidCertificateForAad")
            {
                output = GenericErrorCodes::GenericErrorCodesNoValidCertificateForAad;
                return;
            }
            if (inputStr == "InvalidCertificateForAad")
            {
                output = GenericErrorCodes::GenericErrorCodesInvalidCertificateForAad;
                return;
            }
            if (inputStr == "DuplicateDropTableId")
            {
                output = GenericErrorCodes::GenericErrorCodesDuplicateDropTableId;
                return;
            }
            if (inputStr == "MultiplayerServerError")
            {
                output = GenericErrorCodes::GenericErrorCodesMultiplayerServerError;
                return;
            }
            if (inputStr == "MultiplayerServerTooManyRequests")
            {
                output = GenericErrorCodes::GenericErrorCodesMultiplayerServerTooManyRequests;
                return;
            }
            if (inputStr == "MultiplayerServerNoContent")
            {
                output = GenericErrorCodes::GenericErrorCodesMultiplayerServerNoContent;
                return;
            }
            if (inputStr == "MultiplayerServerBadRequest")
            {
                output = GenericErrorCodes::GenericErrorCodesMultiplayerServerBadRequest;
                return;
            }
            if (inputStr == "MultiplayerServerUnauthorized")
            {
                output = GenericErrorCodes::GenericErrorCodesMultiplayerServerUnauthorized;
                return;
            }
            if (inputStr == "MultiplayerServerForbidden")
            {
                output = GenericErrorCodes::GenericErrorCodesMultiplayerServerForbidden;
                return;
            }
            if (inputStr == "MultiplayerServerNotFound")
            {
                output = GenericErrorCodes::GenericErrorCodesMultiplayerServerNotFound;
                return;
            }
            if (inputStr == "MultiplayerServerConflict")
            {
                output = GenericErrorCodes::GenericErrorCodesMultiplayerServerConflict;
                return;
            }
            if (inputStr == "MultiplayerServerInternalServerError")
            {
                output = GenericErrorCodes::GenericErrorCodesMultiplayerServerInternalServerError;
                return;
            }
            if (inputStr == "MultiplayerServerUnavailable")
            {
                output = GenericErrorCodes::GenericErrorCodesMultiplayerServerUnavailable;
                return;
            }
            if (inputStr == "ExplicitContentDetected")
            {
                output = GenericErrorCodes::GenericErrorCodesExplicitContentDetected;
                return;
            }
            if (inputStr == "PIIContentDetected")
            {
                output = GenericErrorCodes::GenericErrorCodesPIIContentDetected;
                return;
            }
            if (inputStr == "InvalidScheduledTaskParameter")
            {
                output = GenericErrorCodes::GenericErrorCodesInvalidScheduledTaskParameter;
                return;
            }
            if (inputStr == "PerEntityEventRateLimitExceeded")
            {
                output = GenericErrorCodes::GenericErrorCodesPerEntityEventRateLimitExceeded;
                return;
            }
            if (inputStr == "TitleDefaultLanguageNotSet")
            {
                output = GenericErrorCodes::GenericErrorCodesTitleDefaultLanguageNotSet;
                return;
            }
            if (inputStr == "EmailTemplateMissingDefaultVersion")
            {
                output = GenericErrorCodes::GenericErrorCodesEmailTemplateMissingDefaultVersion;
                return;
            }
            if (inputStr == "FacebookInstantGamesIdNotLinked")
            {
                output = GenericErrorCodes::GenericErrorCodesFacebookInstantGamesIdNotLinked;
                return;
            }
            if (inputStr == "InvalidFacebookInstantGamesSignature")
            {
                output = GenericErrorCodes::GenericErrorCodesInvalidFacebookInstantGamesSignature;
                return;
            }
            if (inputStr == "FacebookInstantGamesAuthNotConfiguredForTitle")
            {
                output = GenericErrorCodes::GenericErrorCodesFacebookInstantGamesAuthNotConfiguredForTitle;
                return;
            }
            if (inputStr == "EntityProfileConstraintValidationFailed")
            {
                output = GenericErrorCodes::GenericErrorCodesEntityProfileConstraintValidationFailed;
                return;
            }
            if (inputStr == "TelemetryIngestionKeyPending")
            {
                output = GenericErrorCodes::GenericErrorCodesTelemetryIngestionKeyPending;
                return;
            }
            if (inputStr == "TelemetryIngestionKeyNotFound")
            {
                output = GenericErrorCodes::GenericErrorCodesTelemetryIngestionKeyNotFound;
                return;
            }
            if (inputStr == "StatisticChildNameInvalid")
            {
                output = GenericErrorCodes::GenericErrorCodesStatisticChildNameInvalid;
                return;
            }
            if (inputStr == "DataIntegrityError")
            {
                output = GenericErrorCodes::GenericErrorCodesDataIntegrityError;
                return;
            }
            if (inputStr == "VirtualCurrencyCannotBeSetToOlderVersion")
            {
                output = GenericErrorCodes::GenericErrorCodesVirtualCurrencyCannotBeSetToOlderVersion;
                return;
            }
            if (inputStr == "VirtualCurrencyMustBeWithinIntegerRange")
            {
                output = GenericErrorCodes::GenericErrorCodesVirtualCurrencyMustBeWithinIntegerRange;
                return;
            }
            if (inputStr == "EmailTemplateInvalidSyntax")
            {
                output = GenericErrorCodes::GenericErrorCodesEmailTemplateInvalidSyntax;
                return;
            }
            if (inputStr == "EmailTemplateMissingCallback")
            {
                output = GenericErrorCodes::GenericErrorCodesEmailTemplateMissingCallback;
                return;
            }
            if (inputStr == "PushNotificationTemplateInvalidPayload")
            {
                output = GenericErrorCodes::GenericErrorCodesPushNotificationTemplateInvalidPayload;
                return;
            }
            if (inputStr == "InvalidLocalizedPushNotificationLanguage")
            {
                output = GenericErrorCodes::GenericErrorCodesInvalidLocalizedPushNotificationLanguage;
                return;
            }
            if (inputStr == "MissingLocalizedPushNotificationMessage")
            {
                output = GenericErrorCodes::GenericErrorCodesMissingLocalizedPushNotificationMessage;
                return;
            }
            if (inputStr == "PushNotificationTemplateMissingPlatformPayload")
            {
                output = GenericErrorCodes::GenericErrorCodesPushNotificationTemplateMissingPlatformPayload;
                return;
            }
            if (inputStr == "PushNotificationTemplatePayloadContainsInvalidJson")
            {
                output = GenericErrorCodes::GenericErrorCodesPushNotificationTemplatePayloadContainsInvalidJson;
                return;
            }
            if (inputStr == "PushNotificationTemplateContainsInvalidIosPayload")
            {
                output = GenericErrorCodes::GenericErrorCodesPushNotificationTemplateContainsInvalidIosPayload;
                return;
            }
            if (inputStr == "PushNotificationTemplateContainsInvalidAndroidPayload")
            {
                output = GenericErrorCodes::GenericErrorCodesPushNotificationTemplateContainsInvalidAndroidPayload;
                return;
            }
            if (inputStr == "PushNotificationTemplateIosPayloadMissingNotificationBody")
            {
                output = GenericErrorCodes::GenericErrorCodesPushNotificationTemplateIosPayloadMissingNotificationBody;
                return;
            }
            if (inputStr == "PushNotificationTemplateAndroidPayloadMissingNotificationBody")
            {
                output = GenericErrorCodes::GenericErrorCodesPushNotificationTemplateAndroidPayloadMissingNotificationBody;
                return;
            }
            if (inputStr == "PushNotificationTemplateNotFound")
            {
                output = GenericErrorCodes::GenericErrorCodesPushNotificationTemplateNotFound;
                return;
            }
            if (inputStr == "PushNotificationTemplateMissingDefaultVersion")
            {
                output = GenericErrorCodes::GenericErrorCodesPushNotificationTemplateMissingDefaultVersion;
                return;
            }
            if (inputStr == "PushNotificationTemplateInvalidSyntax")
            {
                output = GenericErrorCodes::GenericErrorCodesPushNotificationTemplateInvalidSyntax;
                return;
            }
            if (inputStr == "PushNotificationTemplateNoCustomPayloadForV1")
            {
                output = GenericErrorCodes::GenericErrorCodesPushNotificationTemplateNoCustomPayloadForV1;
                return;
            }
            if (inputStr == "NoLeaderboardForStatistic")
            {
                output = GenericErrorCodes::GenericErrorCodesNoLeaderboardForStatistic;
                return;
            }
            if (inputStr == "TitleNewsMissingDefaultLanguage")
            {
                output = GenericErrorCodes::GenericErrorCodesTitleNewsMissingDefaultLanguage;
                return;
            }
            if (inputStr == "TitleNewsNotFound")
            {
                output = GenericErrorCodes::GenericErrorCodesTitleNewsNotFound;
                return;
            }
            if (inputStr == "TitleNewsDuplicateLanguage")
            {
                output = GenericErrorCodes::GenericErrorCodesTitleNewsDuplicateLanguage;
                return;
            }
            if (inputStr == "TitleNewsMissingTitleOrBody")
            {
                output = GenericErrorCodes::GenericErrorCodesTitleNewsMissingTitleOrBody;
                return;
            }
            if (inputStr == "TitleNewsInvalidLanguage")
            {
                output = GenericErrorCodes::GenericErrorCodesTitleNewsInvalidLanguage;
                return;
            }
            if (inputStr == "EmailRecipientBlacklisted")
            {
                output = GenericErrorCodes::GenericErrorCodesEmailRecipientBlacklisted;
                return;
            }
            if (inputStr == "InvalidGameCenterAuthRequest")
            {
                output = GenericErrorCodes::GenericErrorCodesInvalidGameCenterAuthRequest;
                return;
            }
            if (inputStr == "GameCenterAuthenticationFailed")
            {
                output = GenericErrorCodes::GenericErrorCodesGameCenterAuthenticationFailed;
                return;
            }
            if (inputStr == "CannotEnablePartiesForTitle")
            {
                output = GenericErrorCodes::GenericErrorCodesCannotEnablePartiesForTitle;
                return;
            }
            if (inputStr == "PartyError")
            {
                output = GenericErrorCodes::GenericErrorCodesPartyError;
                return;
            }
            if (inputStr == "PartyRequests")
            {
                output = GenericErrorCodes::GenericErrorCodesPartyRequests;
                return;
            }
            if (inputStr == "PartyNoContent")
            {
                output = GenericErrorCodes::GenericErrorCodesPartyNoContent;
                return;
            }
            if (inputStr == "PartyBadRequest")
            {
                output = GenericErrorCodes::GenericErrorCodesPartyBadRequest;
                return;
            }
            if (inputStr == "PartyUnauthorized")
            {
                output = GenericErrorCodes::GenericErrorCodesPartyUnauthorized;
                return;
            }
            if (inputStr == "PartyForbidden")
            {
                output = GenericErrorCodes::GenericErrorCodesPartyForbidden;
                return;
            }
            if (inputStr == "PartyNotFound")
            {
                output = GenericErrorCodes::GenericErrorCodesPartyNotFound;
                return;
            }
            if (inputStr == "PartyConflict")
            {
                output = GenericErrorCodes::GenericErrorCodesPartyConflict;
                return;
            }
            if (inputStr == "PartyInternalServerError")
            {
                output = GenericErrorCodes::GenericErrorCodesPartyInternalServerError;
                return;
            }
            if (inputStr == "PartyUnavailable")
            {
                output = GenericErrorCodes::GenericErrorCodesPartyUnavailable;
                return;
            }
            if (inputStr == "PartyTooManyRequests")
            {
                output = GenericErrorCodes::GenericErrorCodesPartyTooManyRequests;
                return;
            }
            if (inputStr == "PushNotificationTemplateMissingName")
            {
                output = GenericErrorCodes::GenericErrorCodesPushNotificationTemplateMissingName;
                return;
            }
            if (inputStr == "CannotEnableMultiplayerServersForTitle")
            {
                output = GenericErrorCodes::GenericErrorCodesCannotEnableMultiplayerServersForTitle;
                return;
            }
            if (inputStr == "WriteAttemptedDuringExport")
            {
                output = GenericErrorCodes::GenericErrorCodesWriteAttemptedDuringExport;
                return;
            }
            if (inputStr == "MultiplayerServerTitleQuotaCoresExceeded")
            {
                output = GenericErrorCodes::GenericErrorCodesMultiplayerServerTitleQuotaCoresExceeded;
                return;
            }
            if (inputStr == "AutomationRuleNotFound")
            {
                output = GenericErrorCodes::GenericErrorCodesAutomationRuleNotFound;
                return;
            }
            if (inputStr == "EntityAPIKeyLimitExceeded")
            {
                output = GenericErrorCodes::GenericErrorCodesEntityAPIKeyLimitExceeded;
                return;
            }
            if (inputStr == "EntityAPIKeyNotFound")
            {
                output = GenericErrorCodes::GenericErrorCodesEntityAPIKeyNotFound;
                return;
            }
            if (inputStr == "EntityAPIKeyOrSecretInvalid")
            {
                output = GenericErrorCodes::GenericErrorCodesEntityAPIKeyOrSecretInvalid;
                return;
            }
            if (inputStr == "EconomyServiceUnavailable")
            {
                output = GenericErrorCodes::GenericErrorCodesEconomyServiceUnavailable;
                return;
            }
            if (inputStr == "EconomyServiceInternalError")
            {
                output = GenericErrorCodes::GenericErrorCodesEconomyServiceInternalError;
                return;
            }
            if (inputStr == "QueryRateLimitExceeded")
            {
                output = GenericErrorCodes::GenericErrorCodesQueryRateLimitExceeded;
                return;
            }
            if (inputStr == "EntityAPIKeyCreationDisabledForEntity")
            {
                output = GenericErrorCodes::GenericErrorCodesEntityAPIKeyCreationDisabledForEntity;
                return;
            }
            if (inputStr == "ForbiddenByEntityPolicy")
            {
                output = GenericErrorCodes::GenericErrorCodesForbiddenByEntityPolicy;
                return;
            }
            if (inputStr == "UpdateInventoryRateLimitExceeded")
            {
                output = GenericErrorCodes::GenericErrorCodesUpdateInventoryRateLimitExceeded;
                return;
            }
            if (inputStr == "StudioCreationRateLimited")
            {
                output = GenericErrorCodes::GenericErrorCodesStudioCreationRateLimited;
                return;
            }
            if (inputStr == "StudioCreationInProgress")
            {
                output = GenericErrorCodes::GenericErrorCodesStudioCreationInProgress;
                return;
            }
            if (inputStr == "DuplicateStudioName")
            {
                output = GenericErrorCodes::GenericErrorCodesDuplicateStudioName;
                return;
            }
            if (inputStr == "StudioNotFound")
            {
                output = GenericErrorCodes::GenericErrorCodesStudioNotFound;
                return;
            }
            if (inputStr == "StudioDeleted")
            {
                output = GenericErrorCodes::GenericErrorCodesStudioDeleted;
                return;
            }
            if (inputStr == "StudioDeactivated")
            {
                output = GenericErrorCodes::GenericErrorCodesStudioDeactivated;
                return;
            }
            if (inputStr == "StudioActivated")
            {
                output = GenericErrorCodes::GenericErrorCodesStudioActivated;
                return;
            }
            if (inputStr == "TitleCreationRateLimited")
            {
                output = GenericErrorCodes::GenericErrorCodesTitleCreationRateLimited;
                return;
            }
            if (inputStr == "TitleCreationInProgress")
            {
                output = GenericErrorCodes::GenericErrorCodesTitleCreationInProgress;
                return;
            }
            if (inputStr == "DuplicateTitleName")
            {
                output = GenericErrorCodes::GenericErrorCodesDuplicateTitleName;
                return;
            }
            if (inputStr == "TitleActivationRateLimited")
            {
                output = GenericErrorCodes::GenericErrorCodesTitleActivationRateLimited;
                return;
            }
            if (inputStr == "TitleActivationInProgress")
            {
                output = GenericErrorCodes::GenericErrorCodesTitleActivationInProgress;
                return;
            }
            if (inputStr == "TitleDeactivated")
            {
                output = GenericErrorCodes::GenericErrorCodesTitleDeactivated;
                return;
            }
            if (inputStr == "TitleActivated")
            {
                output = GenericErrorCodes::GenericErrorCodesTitleActivated;
                return;
            }
            if (inputStr == "CloudScriptAzureFunctionsExecutionTimeLimitExceeded")
            {
                output = GenericErrorCodes::GenericErrorCodesCloudScriptAzureFunctionsExecutionTimeLimitExceeded;
                return;
            }
            if (inputStr == "CloudScriptAzureFunctionsArgumentSizeExceeded")
            {
                output = GenericErrorCodes::GenericErrorCodesCloudScriptAzureFunctionsArgumentSizeExceeded;
                return;
            }
            if (inputStr == "CloudScriptAzureFunctionsReturnSizeExceeded")
            {
                output = GenericErrorCodes::GenericErrorCodesCloudScriptAzureFunctionsReturnSizeExceeded;
                return;
            }
            if (inputStr == "CloudScriptAzureFunctionsHTTPRequestError")
            {
                output = GenericErrorCodes::GenericErrorCodesCloudScriptAzureFunctionsHTTPRequestError;
                return;
            }
            if (inputStr == "VirtualCurrencyBetaGetError")
            {
                output = GenericErrorCodes::GenericErrorCodesVirtualCurrencyBetaGetError;
                return;
            }
            if (inputStr == "VirtualCurrencyBetaCreateError")
            {
                output = GenericErrorCodes::GenericErrorCodesVirtualCurrencyBetaCreateError;
                return;
            }
            if (inputStr == "VirtualCurrencyBetaInitialDepositSaveError")
            {
                output = GenericErrorCodes::GenericErrorCodesVirtualCurrencyBetaInitialDepositSaveError;
                return;
            }
            if (inputStr == "VirtualCurrencyBetaSaveError")
            {
                output = GenericErrorCodes::GenericErrorCodesVirtualCurrencyBetaSaveError;
                return;
            }
            if (inputStr == "VirtualCurrencyBetaDeleteError")
            {
                output = GenericErrorCodes::GenericErrorCodesVirtualCurrencyBetaDeleteError;
                return;
            }
            if (inputStr == "VirtualCurrencyBetaRestoreError")
            {
                output = GenericErrorCodes::GenericErrorCodesVirtualCurrencyBetaRestoreError;
                return;
            }
            if (inputStr == "VirtualCurrencyBetaSaveConflict")
            {
                output = GenericErrorCodes::GenericErrorCodesVirtualCurrencyBetaSaveConflict;
                return;
            }
            if (inputStr == "VirtualCurrencyBetaUpdateError")
            {
                output = GenericErrorCodes::GenericErrorCodesVirtualCurrencyBetaUpdateError;
                return;
            }
            if (inputStr == "InsightsManagementDatabaseNotFound")
            {
                output = GenericErrorCodes::GenericErrorCodesInsightsManagementDatabaseNotFound;
                return;
            }
            if (inputStr == "InsightsManagementOperationNotFound")
            {
                output = GenericErrorCodes::GenericErrorCodesInsightsManagementOperationNotFound;
                return;
            }
            if (inputStr == "InsightsManagementErrorPendingOperationExists")
            {
                output = GenericErrorCodes::GenericErrorCodesInsightsManagementErrorPendingOperationExists;
                return;
            }
            if (inputStr == "InsightsManagementSetPerformanceLevelInvalidParameter")
            {
                output = GenericErrorCodes::GenericErrorCodesInsightsManagementSetPerformanceLevelInvalidParameter;
                return;
            }
            if (inputStr == "InsightsManagementSetStorageRetentionInvalidParameter")
            {
                output = GenericErrorCodes::GenericErrorCodesInsightsManagementSetStorageRetentionInvalidParameter;
                return;
            }
            if (inputStr == "InsightsManagementGetStorageUsageInvalidParameter")
            {
                output = GenericErrorCodes::GenericErrorCodesInsightsManagementGetStorageUsageInvalidParameter;
                return;
            }
            if (inputStr == "InsightsManagementGetOperationStatusInvalidParameter")
            {
                output = GenericErrorCodes::GenericErrorCodesInsightsManagementGetOperationStatusInvalidParameter;
                return;
            }
            if (inputStr == "DuplicatePurchaseTransactionId")
            {
                output = GenericErrorCodes::GenericErrorCodesDuplicatePurchaseTransactionId;
                return;
            }
            if (inputStr == "EvaluationModePlayerCountExceeded")
            {
                output = GenericErrorCodes::GenericErrorCodesEvaluationModePlayerCountExceeded;
                return;
            }
            if (inputStr == "GetPlayersInSegmentRateLimitExceeded")
            {
                output = GenericErrorCodes::GenericErrorCodesGetPlayersInSegmentRateLimitExceeded;
                return;
            }
            if (inputStr == "CloudScriptFunctionNameSizeExceeded")
            {
                output = GenericErrorCodes::GenericErrorCodesCloudScriptFunctionNameSizeExceeded;
                return;
            }
            if (inputStr == "InsightsManagementTitleInEvaluationMode")
            {
                output = GenericErrorCodes::GenericErrorCodesInsightsManagementTitleInEvaluationMode;
                return;
            }
            if (inputStr == "CloudScriptAzureFunctionsQueueRequestError")
            {
                output = GenericErrorCodes::GenericErrorCodesCloudScriptAzureFunctionsQueueRequestError;
                return;
            }
            if (inputStr == "EvaluationModeTitleCountExceeded")
            {
                output = GenericErrorCodes::GenericErrorCodesEvaluationModeTitleCountExceeded;
                return;
            }
            if (inputStr == "InsightsManagementTitleNotInFlight")
            {
                output = GenericErrorCodes::GenericErrorCodesInsightsManagementTitleNotInFlight;
                return;
            }
            if (inputStr == "LimitNotFound")
            {
                output = GenericErrorCodes::GenericErrorCodesLimitNotFound;
                return;
            }
            if (inputStr == "LimitNotAvailableViaAPI")
            {
                output = GenericErrorCodes::GenericErrorCodesLimitNotAvailableViaAPI;
                return;
            }
            if (inputStr == "InsightsManagementSetStorageRetentionBelowMinimum")
            {
                output = GenericErrorCodes::GenericErrorCodesInsightsManagementSetStorageRetentionBelowMinimum;
                return;
            }
            if (inputStr == "InsightsManagementSetStorageRetentionAboveMaximum")
            {
                output = GenericErrorCodes::GenericErrorCodesInsightsManagementSetStorageRetentionAboveMaximum;
                return;
            }
            if (inputStr == "AppleNotEnabledForTitle")
            {
                output = GenericErrorCodes::GenericErrorCodesAppleNotEnabledForTitle;
                return;
            }
            if (inputStr == "InsightsManagementNewActiveEventExportLimitInvalid")
            {
                output = GenericErrorCodes::GenericErrorCodesInsightsManagementNewActiveEventExportLimitInvalid;
                return;
            }
            if (inputStr == "InsightsManagementSetPerformanceRateLimited")
            {
                output = GenericErrorCodes::GenericErrorCodesInsightsManagementSetPerformanceRateLimited;
                return;
            }
            if (inputStr == "PartyRequestsThrottledFromRateLimiter")
            {
                output = GenericErrorCodes::GenericErrorCodesPartyRequestsThrottledFromRateLimiter;
                return;
            }
            if (inputStr == "XboxServiceTooManyRequests")
            {
                output = GenericErrorCodes::GenericErrorCodesXboxServiceTooManyRequests;
                return;
            }
            if (inputStr == "NintendoSwitchNotEnabledForTitle")
            {
                output = GenericErrorCodes::GenericErrorCodesNintendoSwitchNotEnabledForTitle;
                return;
            }
            if (inputStr == "RequestMultiplayerServersThrottledFromRateLimiter")
            {
                output = GenericErrorCodes::GenericErrorCodesRequestMultiplayerServersThrottledFromRateLimiter;
                return;
            }
            if (inputStr == "TitleDataOverrideNotFound")
            {
                output = GenericErrorCodes::GenericErrorCodesTitleDataOverrideNotFound;
                return;
            }
            if (inputStr == "DuplicateKeys")
            {
                output = GenericErrorCodes::GenericErrorCodesDuplicateKeys;
                return;
            }
            if (inputStr == "WasNotCreatedWithCloudRoot")
            {
                output = GenericErrorCodes::GenericErrorCodesWasNotCreatedWithCloudRoot;
                return;
            }
            if (inputStr == "LegacyMultiplayerServersDeprecated")
            {
                output = GenericErrorCodes::GenericErrorCodesLegacyMultiplayerServersDeprecated;
                return;
            }
            if (inputStr == "MatchmakingEntityInvalid")
            {
                output = GenericErrorCodes::GenericErrorCodesMatchmakingEntityInvalid;
                return;
            }
            if (inputStr == "MatchmakingPlayerAttributesInvalid")
            {
                output = GenericErrorCodes::GenericErrorCodesMatchmakingPlayerAttributesInvalid;
                return;
            }
            if (inputStr == "MatchmakingQueueNotFound")
            {
                output = GenericErrorCodes::GenericErrorCodesMatchmakingQueueNotFound;
                return;
            }
            if (inputStr == "MatchmakingMatchNotFound")
            {
                output = GenericErrorCodes::GenericErrorCodesMatchmakingMatchNotFound;
                return;
            }
            if (inputStr == "MatchmakingTicketNotFound")
            {
                output = GenericErrorCodes::GenericErrorCodesMatchmakingTicketNotFound;
                return;
            }
            if (inputStr == "MatchmakingAlreadyJoinedTicket")
            {
                output = GenericErrorCodes::GenericErrorCodesMatchmakingAlreadyJoinedTicket;
                return;
            }
            if (inputStr == "MatchmakingTicketAlreadyCompleted")
            {
                output = GenericErrorCodes::GenericErrorCodesMatchmakingTicketAlreadyCompleted;
                return;
            }
            if (inputStr == "MatchmakingQueueConfigInvalid")
            {
                output = GenericErrorCodes::GenericErrorCodesMatchmakingQueueConfigInvalid;
                return;
            }
            if (inputStr == "MatchmakingMemberProfileInvalid")
            {
                output = GenericErrorCodes::GenericErrorCodesMatchmakingMemberProfileInvalid;
                return;
            }
            if (inputStr == "NintendoSwitchDeviceIdNotLinked")
            {
                output = GenericErrorCodes::GenericErrorCodesNintendoSwitchDeviceIdNotLinked;
                return;
            }
            if (inputStr == "MatchmakingNotEnabled")
            {
                output = GenericErrorCodes::GenericErrorCodesMatchmakingNotEnabled;
                return;
            }
            if (inputStr == "MatchmakingPlayerAttributesTooLarge")
            {
                output = GenericErrorCodes::GenericErrorCodesMatchmakingPlayerAttributesTooLarge;
                return;
            }
            if (inputStr == "MatchmakingNumberOfPlayersInTicketTooLarge")
            {
                output = GenericErrorCodes::GenericErrorCodesMatchmakingNumberOfPlayersInTicketTooLarge;
                return;
            }
            if (inputStr == "MatchmakingAttributeInvalid")
            {
                output = GenericErrorCodes::GenericErrorCodesMatchmakingAttributeInvalid;
                return;
            }
            if (inputStr == "MatchmakingPlayerHasNotJoinedTicket")
            {
                output = GenericErrorCodes::GenericErrorCodesMatchmakingPlayerHasNotJoinedTicket;
                return;
            }
            if (inputStr == "MatchmakingRateLimitExceeded")
            {
                output = GenericErrorCodes::GenericErrorCodesMatchmakingRateLimitExceeded;
                return;
            }
            if (inputStr == "MatchmakingTicketMembershipLimitExceeded")
            {
                output = GenericErrorCodes::GenericErrorCodesMatchmakingTicketMembershipLimitExceeded;
                return;
            }
            if (inputStr == "MatchmakingUnauthorized")
            {
                output = GenericErrorCodes::GenericErrorCodesMatchmakingUnauthorized;
                return;
            }
            if (inputStr == "MatchmakingQueueLimitExceeded")
            {
                output = GenericErrorCodes::GenericErrorCodesMatchmakingQueueLimitExceeded;
                return;
            }
            if (inputStr == "MatchmakingRequestTypeMismatch")
            {
                output = GenericErrorCodes::GenericErrorCodesMatchmakingRequestTypeMismatch;
                return;
            }
            if (inputStr == "MatchmakingBadRequest")
            {
                output = GenericErrorCodes::GenericErrorCodesMatchmakingBadRequest;
                return;
            }
            if (inputStr == "TitleConfigNotFound")
            {
                output = GenericErrorCodes::GenericErrorCodesTitleConfigNotFound;
                return;
            }
            if (inputStr == "TitleConfigUpdateConflict")
            {
                output = GenericErrorCodes::GenericErrorCodesTitleConfigUpdateConflict;
                return;
            }
            if (inputStr == "TitleConfigSerializationError")
            {
                output = GenericErrorCodes::GenericErrorCodesTitleConfigSerializationError;
                return;
            }
            if (inputStr == "CatalogEntityInvalid")
            {
                output = GenericErrorCodes::GenericErrorCodesCatalogEntityInvalid;
                return;
            }
            if (inputStr == "CatalogTitleIdMissing")
            {
                output = GenericErrorCodes::GenericErrorCodesCatalogTitleIdMissing;
                return;
            }
            if (inputStr == "CatalogPlayerIdMissing")
            {
                output = GenericErrorCodes::GenericErrorCodesCatalogPlayerIdMissing;
                return;
            }
            if (inputStr == "CatalogClientIdentityInvalid")
            {
                output = GenericErrorCodes::GenericErrorCodesCatalogClientIdentityInvalid;
                return;
            }
            if (inputStr == "CatalogOneOrMoreFilesInvalid")
            {
                output = GenericErrorCodes::GenericErrorCodesCatalogOneOrMoreFilesInvalid;
                return;
            }
            if (inputStr == "CatalogItemMetadataInvalid")
            {
                output = GenericErrorCodes::GenericErrorCodesCatalogItemMetadataInvalid;
                return;
            }
            if (inputStr == "CatalogItemIdInvalid")
            {
                output = GenericErrorCodes::GenericErrorCodesCatalogItemIdInvalid;
                return;
            }
            if (inputStr == "CatalogSearchParameterInvalid")
            {
                output = GenericErrorCodes::GenericErrorCodesCatalogSearchParameterInvalid;
                return;
            }
            if (inputStr == "CatalogFeatureDisabled")
            {
                output = GenericErrorCodes::GenericErrorCodesCatalogFeatureDisabled;
                return;
            }
            if (inputStr == "CatalogConfigInvalid")
            {
                output = GenericErrorCodes::GenericErrorCodesCatalogConfigInvalid;
                return;
            }
            if (inputStr == "CatalogUnauthorized")
            {
                output = GenericErrorCodes::GenericErrorCodesCatalogUnauthorized;
                return;
            }
            if (inputStr == "CatalogItemTypeInvalid")
            {
                output = GenericErrorCodes::GenericErrorCodesCatalogItemTypeInvalid;
                return;
            }
            if (inputStr == "CatalogBadRequest")
            {
                output = GenericErrorCodes::GenericErrorCodesCatalogBadRequest;
                return;
            }
            if (inputStr == "CatalogTooManyRequests")
            {
                output = GenericErrorCodes::GenericErrorCodesCatalogTooManyRequests;
                return;
            }
            if (inputStr == "ExportInvalidStatusUpdate")
            {
                output = GenericErrorCodes::GenericErrorCodesExportInvalidStatusUpdate;
                return;
            }
            if (inputStr == "ExportInvalidPrefix")
            {
                output = GenericErrorCodes::GenericErrorCodesExportInvalidPrefix;
                return;
            }
            if (inputStr == "ExportBlobContainerDoesNotExist")
            {
                output = GenericErrorCodes::GenericErrorCodesExportBlobContainerDoesNotExist;
                return;
            }
            if (inputStr == "ExportNotFound")
            {
                output = GenericErrorCodes::GenericErrorCodesExportNotFound;
                return;
            }
            if (inputStr == "ExportCouldNotUpdate")
            {
                output = GenericErrorCodes::GenericErrorCodesExportCouldNotUpdate;
                return;
            }
            if (inputStr == "ExportInvalidStorageType")
            {
                output = GenericErrorCodes::GenericErrorCodesExportInvalidStorageType;
                return;
            }
            if (inputStr == "ExportAmazonBucketDoesNotExist")
            {
                output = GenericErrorCodes::GenericErrorCodesExportAmazonBucketDoesNotExist;
                return;
            }
            if (inputStr == "ExportInvalidBlobStorage")
            {
                output = GenericErrorCodes::GenericErrorCodesExportInvalidBlobStorage;
                return;
            }
            if (inputStr == "ExportKustoException")
            {
                output = GenericErrorCodes::GenericErrorCodesExportKustoException;
                return;
            }
            if (inputStr == "ExportKustoConnectionFailed")
            {
                output = GenericErrorCodes::GenericErrorCodesExportKustoConnectionFailed;
                return;
            }
            if (inputStr == "ExportUnknownError")
            {
                output = GenericErrorCodes::GenericErrorCodesExportUnknownError;
                return;
            }
            if (inputStr == "ExportCantEditPendingExport")
            {
                output = GenericErrorCodes::GenericErrorCodesExportCantEditPendingExport;
                return;
            }
            if (inputStr == "ExportLimitExports")
            {
                output = GenericErrorCodes::GenericErrorCodesExportLimitExports;
                return;
            }
            if (inputStr == "ExportLimitEvents")
            {
                output = GenericErrorCodes::GenericErrorCodesExportLimitEvents;
                return;
            }
            if (inputStr == "ExportInvalidPartitionStatusModification")
            {
                output = GenericErrorCodes::GenericErrorCodesExportInvalidPartitionStatusModification;
                return;
            }
            if (inputStr == "ExportCouldNotCreate")
            {
                output = GenericErrorCodes::GenericErrorCodesExportCouldNotCreate;
                return;
            }
            if (inputStr == "ExportNoBackingDatabaseFound")
            {
                output = GenericErrorCodes::GenericErrorCodesExportNoBackingDatabaseFound;
                return;
            }
            if (inputStr == "ExportCouldNotDelete")
            {
                output = GenericErrorCodes::GenericErrorCodesExportCouldNotDelete;
                return;
            }
            if (inputStr == "ExportCannotDetermineEventQuery")
            {
                output = GenericErrorCodes::GenericErrorCodesExportCannotDetermineEventQuery;
                return;
            }
            if (inputStr == "ExportInvalidQuerySchemaModification")
            {
                output = GenericErrorCodes::GenericErrorCodesExportInvalidQuerySchemaModification;
                return;
            }
            if (inputStr == "ExportQuerySchemaMissingRequiredColumns")
            {
                output = GenericErrorCodes::GenericErrorCodesExportQuerySchemaMissingRequiredColumns;
                return;
            }
            if (inputStr == "ExportCannotParseQuery")
            {
                output = GenericErrorCodes::GenericErrorCodesExportCannotParseQuery;
                return;
            }
            if (inputStr == "ExportControlCommandsNotAllowed")
            {
                output = GenericErrorCodes::GenericErrorCodesExportControlCommandsNotAllowed;
                return;
            }
            if (inputStr == "ExportQueryMissingTableReference")
            {
                output = GenericErrorCodes::GenericErrorCodesExportQueryMissingTableReference;
                return;
            }
            if (inputStr == "TitleNotEnabledForParty")
            {
                output = GenericErrorCodes::GenericErrorCodesTitleNotEnabledForParty;
                return;
            }
            if (inputStr == "PartyVersionNotFound")
            {
                output = GenericErrorCodes::GenericErrorCodesPartyVersionNotFound;
                return;
            }
            if (inputStr == "MultiplayerServerBuildReferencedByMatchmakingQueue")
            {
                output = GenericErrorCodes::GenericErrorCodesMultiplayerServerBuildReferencedByMatchmakingQueue;
                return;
            }
            if (inputStr == "ExperimentationExperimentStopped")
            {
                output = GenericErrorCodes::GenericErrorCodesExperimentationExperimentStopped;
                return;
            }
            if (inputStr == "ExperimentationExperimentRunning")
            {
                output = GenericErrorCodes::GenericErrorCodesExperimentationExperimentRunning;
                return;
            }
            if (inputStr == "ExperimentationExperimentNotFound")
            {
                output = GenericErrorCodes::GenericErrorCodesExperimentationExperimentNotFound;
                return;
            }
            if (inputStr == "ExperimentationExperimentNeverStarted")
            {
                output = GenericErrorCodes::GenericErrorCodesExperimentationExperimentNeverStarted;
                return;
            }
            if (inputStr == "ExperimentationExperimentDeleted")
            {
                output = GenericErrorCodes::GenericErrorCodesExperimentationExperimentDeleted;
                return;
            }
            if (inputStr == "ExperimentationClientTimeout")
            {
                output = GenericErrorCodes::GenericErrorCodesExperimentationClientTimeout;
                return;
            }
            if (inputStr == "ExperimentationInvalidVariantConfiguration")
            {
                output = GenericErrorCodes::GenericErrorCodesExperimentationInvalidVariantConfiguration;
                return;
            }
            if (inputStr == "ExperimentationInvalidVariableConfiguration")
            {
                output = GenericErrorCodes::GenericErrorCodesExperimentationInvalidVariableConfiguration;
                return;
            }
            if (inputStr == "ExperimentInvalidId")
            {
                output = GenericErrorCodes::GenericErrorCodesExperimentInvalidId;
                return;
            }
            if (inputStr == "ExperimentationNoScorecard")
            {
                output = GenericErrorCodes::GenericErrorCodesExperimentationNoScorecard;
                return;
            }
            if (inputStr == "ExperimentationTreatmentAssignmentFailed")
            {
                output = GenericErrorCodes::GenericErrorCodesExperimentationTreatmentAssignmentFailed;
                return;
            }
            if (inputStr == "ExperimentationTreatmentAssignmentDisabled")
            {
                output = GenericErrorCodes::GenericErrorCodesExperimentationTreatmentAssignmentDisabled;
                return;
            }
            if (inputStr == "ExperimentationInvalidDuration")
            {
                output = GenericErrorCodes::GenericErrorCodesExperimentationInvalidDuration;
                return;
            }
            if (inputStr == "ExperimentationMaxExperimentsReached")
            {
                output = GenericErrorCodes::GenericErrorCodesExperimentationMaxExperimentsReached;
                return;
            }
            if (inputStr == "ExperimentationExperimentSchedulingInProgress")
            {
                output = GenericErrorCodes::GenericErrorCodesExperimentationExperimentSchedulingInProgress;
                return;
            }
            if (inputStr == "MaxActionDepthExceeded")
            {
                output = GenericErrorCodes::GenericErrorCodesMaxActionDepthExceeded;
                return;
            }
            if (inputStr == "TitleNotOnUpdatedPricingPlan")
            {
                output = GenericErrorCodes::GenericErrorCodesTitleNotOnUpdatedPricingPlan;
                return;
            }
            if (inputStr == "SnapshotNotFound")
            {
                output = GenericErrorCodes::GenericErrorCodesSnapshotNotFound;
                return;
            }
        }

        enum class LoginIdentityProvider
        {
            LoginIdentityProviderUnknown,
            LoginIdentityProviderPlayFab,
            LoginIdentityProviderCustom,
            LoginIdentityProviderGameCenter,
            LoginIdentityProviderGooglePlay,
            LoginIdentityProviderSteam,
            LoginIdentityProviderXBoxLive,
            LoginIdentityProviderPSN,
            LoginIdentityProviderKongregate,
            LoginIdentityProviderFacebook,
            LoginIdentityProviderIOSDevice,
            LoginIdentityProviderAndroidDevice,
            LoginIdentityProviderTwitch,
            LoginIdentityProviderWindowsHello,
            LoginIdentityProviderGameServer,
            LoginIdentityProviderCustomServer,
            LoginIdentityProviderNintendoSwitch,
            LoginIdentityProviderFacebookInstantGames,
            LoginIdentityProviderOpenIdConnect,
            LoginIdentityProviderApple,
            LoginIdentityProviderNintendoSwitchAccount
        };

        inline void ToJsonEnum(const LoginIdentityProvider input, Json::Value& output)
        {
            if (input == LoginIdentityProvider::LoginIdentityProviderUnknown)
            {
                output = Json::Value("Unknown");
                return;
            }
            if (input == LoginIdentityProvider::LoginIdentityProviderPlayFab)
            {
                output = Json::Value("PlayFab");
                return;
            }
            if (input == LoginIdentityProvider::LoginIdentityProviderCustom)
            {
                output = Json::Value("Custom");
                return;
            }
            if (input == LoginIdentityProvider::LoginIdentityProviderGameCenter)
            {
                output = Json::Value("GameCenter");
                return;
            }
            if (input == LoginIdentityProvider::LoginIdentityProviderGooglePlay)
            {
                output = Json::Value("GooglePlay");
                return;
            }
            if (input == LoginIdentityProvider::LoginIdentityProviderSteam)
            {
                output = Json::Value("Steam");
                return;
            }
            if (input == LoginIdentityProvider::LoginIdentityProviderXBoxLive)
            {
                output = Json::Value("XBoxLive");
                return;
            }
            if (input == LoginIdentityProvider::LoginIdentityProviderPSN)
            {
                output = Json::Value("PSN");
                return;
            }
            if (input == LoginIdentityProvider::LoginIdentityProviderKongregate)
            {
                output = Json::Value("Kongregate");
                return;
            }
            if (input == LoginIdentityProvider::LoginIdentityProviderFacebook)
            {
                output = Json::Value("Facebook");
                return;
            }
            if (input == LoginIdentityProvider::LoginIdentityProviderIOSDevice)
            {
                output = Json::Value("IOSDevice");
                return;
            }
            if (input == LoginIdentityProvider::LoginIdentityProviderAndroidDevice)
            {
                output = Json::Value("AndroidDevice");
                return;
            }
            if (input == LoginIdentityProvider::LoginIdentityProviderTwitch)
            {
                output = Json::Value("Twitch");
                return;
            }
            if (input == LoginIdentityProvider::LoginIdentityProviderWindowsHello)
            {
                output = Json::Value("WindowsHello");
                return;
            }
            if (input == LoginIdentityProvider::LoginIdentityProviderGameServer)
            {
                output = Json::Value("GameServer");
                return;
            }
            if (input == LoginIdentityProvider::LoginIdentityProviderCustomServer)
            {
                output = Json::Value("CustomServer");
                return;
            }
            if (input == LoginIdentityProvider::LoginIdentityProviderNintendoSwitch)
            {
                output = Json::Value("NintendoSwitch");
                return;
            }
            if (input == LoginIdentityProvider::LoginIdentityProviderFacebookInstantGames)
            {
                output = Json::Value("FacebookInstantGames");
                return;
            }
            if (input == LoginIdentityProvider::LoginIdentityProviderOpenIdConnect)
            {
                output = Json::Value("OpenIdConnect");
                return;
            }
            if (input == LoginIdentityProvider::LoginIdentityProviderApple)
            {
                output = Json::Value("Apple");
                return;
            }
            if (input == LoginIdentityProvider::LoginIdentityProviderNintendoSwitchAccount)
            {
                output = Json::Value("NintendoSwitchAccount");
                return;
            }
        }
        inline void FromJsonEnum(const Json::Value& input, LoginIdentityProvider& output)
        {
            if (!input.isString())
            {
                return;
            }
            const std::string& inputStr = input.asString();
            if (inputStr == "Unknown")
            {
                output = LoginIdentityProvider::LoginIdentityProviderUnknown;
                return;
            }
            if (inputStr == "PlayFab")
            {
                output = LoginIdentityProvider::LoginIdentityProviderPlayFab;
                return;
            }
            if (inputStr == "Custom")
            {
                output = LoginIdentityProvider::LoginIdentityProviderCustom;
                return;
            }
            if (inputStr == "GameCenter")
            {
                output = LoginIdentityProvider::LoginIdentityProviderGameCenter;
                return;
            }
            if (inputStr == "GooglePlay")
            {
                output = LoginIdentityProvider::LoginIdentityProviderGooglePlay;
                return;
            }
            if (inputStr == "Steam")
            {
                output = LoginIdentityProvider::LoginIdentityProviderSteam;
                return;
            }
            if (inputStr == "XBoxLive")
            {
                output = LoginIdentityProvider::LoginIdentityProviderXBoxLive;
                return;
            }
            if (inputStr == "PSN")
            {
                output = LoginIdentityProvider::LoginIdentityProviderPSN;
                return;
            }
            if (inputStr == "Kongregate")
            {
                output = LoginIdentityProvider::LoginIdentityProviderKongregate;
                return;
            }
            if (inputStr == "Facebook")
            {
                output = LoginIdentityProvider::LoginIdentityProviderFacebook;
                return;
            }
            if (inputStr == "IOSDevice")
            {
                output = LoginIdentityProvider::LoginIdentityProviderIOSDevice;
                return;
            }
            if (inputStr == "AndroidDevice")
            {
                output = LoginIdentityProvider::LoginIdentityProviderAndroidDevice;
                return;
            }
            if (inputStr == "Twitch")
            {
                output = LoginIdentityProvider::LoginIdentityProviderTwitch;
                return;
            }
            if (inputStr == "WindowsHello")
            {
                output = LoginIdentityProvider::LoginIdentityProviderWindowsHello;
                return;
            }
            if (inputStr == "GameServer")
            {
                output = LoginIdentityProvider::LoginIdentityProviderGameServer;
                return;
            }
            if (inputStr == "CustomServer")
            {
                output = LoginIdentityProvider::LoginIdentityProviderCustomServer;
                return;
            }
            if (inputStr == "NintendoSwitch")
            {
                output = LoginIdentityProvider::LoginIdentityProviderNintendoSwitch;
                return;
            }
            if (inputStr == "FacebookInstantGames")
            {
                output = LoginIdentityProvider::LoginIdentityProviderFacebookInstantGames;
                return;
            }
            if (inputStr == "OpenIdConnect")
            {
                output = LoginIdentityProvider::LoginIdentityProviderOpenIdConnect;
                return;
            }
            if (inputStr == "Apple")
            {
                output = LoginIdentityProvider::LoginIdentityProviderApple;
                return;
            }
            if (inputStr == "NintendoSwitchAccount")
            {
                output = LoginIdentityProvider::LoginIdentityProviderNintendoSwitchAccount;
                return;
            }
        }

        enum class PushNotificationPlatform
        {
            PushNotificationPlatformApplePushNotificationService,
            PushNotificationPlatformGoogleCloudMessaging
        };

        inline void ToJsonEnum(const PushNotificationPlatform input, Json::Value& output)
        {
            if (input == PushNotificationPlatform::PushNotificationPlatformApplePushNotificationService)
            {
                output = Json::Value("ApplePushNotificationService");
                return;
            }
            if (input == PushNotificationPlatform::PushNotificationPlatformGoogleCloudMessaging)
            {
                output = Json::Value("GoogleCloudMessaging");
                return;
            }
        }
        inline void FromJsonEnum(const Json::Value& input, PushNotificationPlatform& output)
        {
            if (!input.isString())
            {
                return;
            }
            const std::string& inputStr = input.asString();
            if (inputStr == "ApplePushNotificationService")
            {
                output = PushNotificationPlatform::PushNotificationPlatformApplePushNotificationService;
                return;
            }
            if (inputStr == "GoogleCloudMessaging")
            {
                output = PushNotificationPlatform::PushNotificationPlatformGoogleCloudMessaging;
                return;
            }
        }

        enum class PushSetupPlatform
        {
            PushSetupPlatformGCM,
            PushSetupPlatformAPNS,
            PushSetupPlatformAPNS_SANDBOX
        };

        inline void ToJsonEnum(const PushSetupPlatform input, Json::Value& output)
        {
            if (input == PushSetupPlatform::PushSetupPlatformGCM)
            {
                output = Json::Value("GCM");
                return;
            }
            if (input == PushSetupPlatform::PushSetupPlatformAPNS)
            {
                output = Json::Value("APNS");
                return;
            }
            if (input == PushSetupPlatform::PushSetupPlatformAPNS_SANDBOX)
            {
                output = Json::Value("APNS_SANDBOX");
                return;
            }
        }
        inline void FromJsonEnum(const Json::Value& input, PushSetupPlatform& output)
        {
            if (!input.isString())
            {
                return;
            }
            const std::string& inputStr = input.asString();
            if (inputStr == "GCM")
            {
                output = PushSetupPlatform::PushSetupPlatformGCM;
                return;
            }
            if (inputStr == "APNS")
            {
                output = PushSetupPlatform::PushSetupPlatformAPNS;
                return;
            }
            if (inputStr == "APNS_SANDBOX")
            {
                output = PushSetupPlatform::PushSetupPlatformAPNS_SANDBOX;
                return;
            }
        }

        enum class Region
        {
            RegionUSCentral,
            RegionUSEast,
            RegionEUWest,
            RegionSingapore,
            RegionJapan,
            RegionBrazil,
            RegionAustralia
        };

        inline void ToJsonEnum(const Region input, Json::Value& output)
        {
            if (input == Region::RegionUSCentral)
            {
                output = Json::Value("USCentral");
                return;
            }
            if (input == Region::RegionUSEast)
            {
                output = Json::Value("USEast");
                return;
            }
            if (input == Region::RegionEUWest)
            {
                output = Json::Value("EUWest");
                return;
            }
            if (input == Region::RegionSingapore)
            {
                output = Json::Value("Singapore");
                return;
            }
            if (input == Region::RegionJapan)
            {
                output = Json::Value("Japan");
                return;
            }
            if (input == Region::RegionBrazil)
            {
                output = Json::Value("Brazil");
                return;
            }
            if (input == Region::RegionAustralia)
            {
                output = Json::Value("Australia");
                return;
            }
        }
        inline void FromJsonEnum(const Json::Value& input, Region& output)
        {
            if (!input.isString())
            {
                return;
            }
            const std::string& inputStr = input.asString();
            if (inputStr == "USCentral")
            {
                output = Region::RegionUSCentral;
                return;
            }
            if (inputStr == "USEast")
            {
                output = Region::RegionUSEast;
                return;
            }
            if (inputStr == "EUWest")
            {
                output = Region::RegionEUWest;
                return;
            }
            if (inputStr == "Singapore")
            {
                output = Region::RegionSingapore;
                return;
            }
            if (inputStr == "Japan")
            {
                output = Region::RegionJapan;
                return;
            }
            if (inputStr == "Brazil")
            {
                output = Region::RegionBrazil;
                return;
            }
            if (inputStr == "Australia")
            {
                output = Region::RegionAustralia;
                return;
            }
        }

        enum class ResolutionOutcome
        {
            ResolutionOutcomeRevoke,
            ResolutionOutcomeReinstate,
            ResolutionOutcomeManual
        };

        inline void ToJsonEnum(const ResolutionOutcome input, Json::Value& output)
        {
            if (input == ResolutionOutcome::ResolutionOutcomeRevoke)
            {
                output = Json::Value("Revoke");
                return;
            }
            if (input == ResolutionOutcome::ResolutionOutcomeReinstate)
            {
                output = Json::Value("Reinstate");
                return;
            }
            if (input == ResolutionOutcome::ResolutionOutcomeManual)
            {
                output = Json::Value("Manual");
                return;
            }
        }
        inline void FromJsonEnum(const Json::Value& input, ResolutionOutcome& output)
        {
            if (!input.isString())
            {
                return;
            }
            const std::string& inputStr = input.asString();
            if (inputStr == "Revoke")
            {
                output = ResolutionOutcome::ResolutionOutcomeRevoke;
                return;
            }
            if (inputStr == "Reinstate")
            {
                output = ResolutionOutcome::ResolutionOutcomeReinstate;
                return;
            }
            if (inputStr == "Manual")
            {
                output = ResolutionOutcome::ResolutionOutcomeManual;
                return;
            }
        }

        enum class ResultTableNodeType
        {
            ResultTableNodeTypeItemId,
            ResultTableNodeTypeTableId
        };

        inline void ToJsonEnum(const ResultTableNodeType input, Json::Value& output)
        {
            if (input == ResultTableNodeType::ResultTableNodeTypeItemId)
            {
                output = Json::Value("ItemId");
                return;
            }
            if (input == ResultTableNodeType::ResultTableNodeTypeTableId)
            {
                output = Json::Value("TableId");
                return;
            }
        }
        inline void FromJsonEnum(const Json::Value& input, ResultTableNodeType& output)
        {
            if (!input.isString())
            {
                return;
            }
            const std::string& inputStr = input.asString();
            if (inputStr == "ItemId")
            {
                output = ResultTableNodeType::ResultTableNodeTypeItemId;
                return;
            }
            if (inputStr == "TableId")
            {
                output = ResultTableNodeType::ResultTableNodeTypeTableId;
                return;
            }
        }

        enum class ScheduledTaskType
        {
            ScheduledTaskTypeCloudScript,
            ScheduledTaskTypeActionsOnPlayerSegment,
            ScheduledTaskTypeCloudScriptAzureFunctions,
            ScheduledTaskTypeInsightsScheduledScaling
        };

        inline void ToJsonEnum(const ScheduledTaskType input, Json::Value& output)
        {
            if (input == ScheduledTaskType::ScheduledTaskTypeCloudScript)
            {
                output = Json::Value("CloudScript");
                return;
            }
            if (input == ScheduledTaskType::ScheduledTaskTypeActionsOnPlayerSegment)
            {
                output = Json::Value("ActionsOnPlayerSegment");
                return;
            }
            if (input == ScheduledTaskType::ScheduledTaskTypeCloudScriptAzureFunctions)
            {
                output = Json::Value("CloudScriptAzureFunctions");
                return;
            }
            if (input == ScheduledTaskType::ScheduledTaskTypeInsightsScheduledScaling)
            {
                output = Json::Value("InsightsScheduledScaling");
                return;
            }
        }
        inline void FromJsonEnum(const Json::Value& input, ScheduledTaskType& output)
        {
            if (!input.isString())
            {
                return;
            }
            const std::string& inputStr = input.asString();
            if (inputStr == "CloudScript")
            {
                output = ScheduledTaskType::ScheduledTaskTypeCloudScript;
                return;
            }
            if (inputStr == "ActionsOnPlayerSegment")
            {
                output = ScheduledTaskType::ScheduledTaskTypeActionsOnPlayerSegment;
                return;
            }
            if (inputStr == "CloudScriptAzureFunctions")
            {
                output = ScheduledTaskType::ScheduledTaskTypeCloudScriptAzureFunctions;
                return;
            }
            if (inputStr == "InsightsScheduledScaling")
            {
                output = ScheduledTaskType::ScheduledTaskTypeInsightsScheduledScaling;
                return;
            }
        }

        enum class SourceType
        {
            SourceTypeAdmin,
            SourceTypeBackEnd,
            SourceTypeGameClient,
            SourceTypeGameServer,
            SourceTypePartner,
            SourceTypeCustom,
            SourceTypeAPI
        };

        inline void ToJsonEnum(const SourceType input, Json::Value& output)
        {
            if (input == SourceType::SourceTypeAdmin)
            {
                output = Json::Value("Admin");
                return;
            }
            if (input == SourceType::SourceTypeBackEnd)
            {
                output = Json::Value("BackEnd");
                return;
            }
            if (input == SourceType::SourceTypeGameClient)
            {
                output = Json::Value("GameClient");
                return;
            }
            if (input == SourceType::SourceTypeGameServer)
            {
                output = Json::Value("GameServer");
                return;
            }
            if (input == SourceType::SourceTypePartner)
            {
                output = Json::Value("Partner");
                return;
            }
            if (input == SourceType::SourceTypeCustom)
            {
                output = Json::Value("Custom");
                return;
            }
            if (input == SourceType::SourceTypeAPI)
            {
                output = Json::Value("API");
                return;
            }
        }
        inline void FromJsonEnum(const Json::Value& input, SourceType& output)
        {
            if (!input.isString())
            {
                return;
            }
            const std::string& inputStr = input.asString();
            if (inputStr == "Admin")
            {
                output = SourceType::SourceTypeAdmin;
                return;
            }
            if (inputStr == "BackEnd")
            {
                output = SourceType::SourceTypeBackEnd;
                return;
            }
            if (inputStr == "GameClient")
            {
                output = SourceType::SourceTypeGameClient;
                return;
            }
            if (inputStr == "GameServer")
            {
                output = SourceType::SourceTypeGameServer;
                return;
            }
            if (inputStr == "Partner")
            {
                output = SourceType::SourceTypePartner;
                return;
            }
            if (inputStr == "Custom")
            {
                output = SourceType::SourceTypeCustom;
                return;
            }
            if (inputStr == "API")
            {
                output = SourceType::SourceTypeAPI;
                return;
            }
        }

        enum class StatisticAggregationMethod
        {
            StatisticAggregationMethodLast,
            StatisticAggregationMethodMin,
            StatisticAggregationMethodMax,
            StatisticAggregationMethodSum
        };

        inline void ToJsonEnum(const StatisticAggregationMethod input, Json::Value& output)
        {
            if (input == StatisticAggregationMethod::StatisticAggregationMethodLast)
            {
                output = Json::Value("Last");
                return;
            }
            if (input == StatisticAggregationMethod::StatisticAggregationMethodMin)
            {
                output = Json::Value("Min");
                return;
            }
            if (input == StatisticAggregationMethod::StatisticAggregationMethodMax)
            {
                output = Json::Value("Max");
                return;
            }
            if (input == StatisticAggregationMethod::StatisticAggregationMethodSum)
            {
                output = Json::Value("Sum");
                return;
            }
        }
        inline void FromJsonEnum(const Json::Value& input, StatisticAggregationMethod& output)
        {
            if (!input.isString())
            {
                return;
            }
            const std::string& inputStr = input.asString();
            if (inputStr == "Last")
            {
                output = StatisticAggregationMethod::StatisticAggregationMethodLast;
                return;
            }
            if (inputStr == "Min")
            {
                output = StatisticAggregationMethod::StatisticAggregationMethodMin;
                return;
            }
            if (inputStr == "Max")
            {
                output = StatisticAggregationMethod::StatisticAggregationMethodMax;
                return;
            }
            if (inputStr == "Sum")
            {
                output = StatisticAggregationMethod::StatisticAggregationMethodSum;
                return;
            }
        }

        enum class StatisticResetIntervalOption
        {
            StatisticResetIntervalOptionNever,
            StatisticResetIntervalOptionHour,
            StatisticResetIntervalOptionDay,
            StatisticResetIntervalOptionWeek,
            StatisticResetIntervalOptionMonth
        };

        inline void ToJsonEnum(const StatisticResetIntervalOption input, Json::Value& output)
        {
            if (input == StatisticResetIntervalOption::StatisticResetIntervalOptionNever)
            {
                output = Json::Value("Never");
                return;
            }
            if (input == StatisticResetIntervalOption::StatisticResetIntervalOptionHour)
            {
                output = Json::Value("Hour");
                return;
            }
            if (input == StatisticResetIntervalOption::StatisticResetIntervalOptionDay)
            {
                output = Json::Value("Day");
                return;
            }
            if (input == StatisticResetIntervalOption::StatisticResetIntervalOptionWeek)
            {
                output = Json::Value("Week");
                return;
            }
            if (input == StatisticResetIntervalOption::StatisticResetIntervalOptionMonth)
            {
                output = Json::Value("Month");
                return;
            }
        }
        inline void FromJsonEnum(const Json::Value& input, StatisticResetIntervalOption& output)
        {
            if (!input.isString())
            {
                return;
            }
            const std::string& inputStr = input.asString();
            if (inputStr == "Never")
            {
                output = StatisticResetIntervalOption::StatisticResetIntervalOptionNever;
                return;
            }
            if (inputStr == "Hour")
            {
                output = StatisticResetIntervalOption::StatisticResetIntervalOptionHour;
                return;
            }
            if (inputStr == "Day")
            {
                output = StatisticResetIntervalOption::StatisticResetIntervalOptionDay;
                return;
            }
            if (inputStr == "Week")
            {
                output = StatisticResetIntervalOption::StatisticResetIntervalOptionWeek;
                return;
            }
            if (inputStr == "Month")
            {
                output = StatisticResetIntervalOption::StatisticResetIntervalOptionMonth;
                return;
            }
        }

        enum class StatisticVersionArchivalStatus
        {
            StatisticVersionArchivalStatusNotScheduled,
            StatisticVersionArchivalStatusScheduled,
            StatisticVersionArchivalStatusQueued,
            StatisticVersionArchivalStatusInProgress,
            StatisticVersionArchivalStatusComplete
        };

        inline void ToJsonEnum(const StatisticVersionArchivalStatus input, Json::Value& output)
        {
            if (input == StatisticVersionArchivalStatus::StatisticVersionArchivalStatusNotScheduled)
            {
                output = Json::Value("NotScheduled");
                return;
            }
            if (input == StatisticVersionArchivalStatus::StatisticVersionArchivalStatusScheduled)
            {
                output = Json::Value("Scheduled");
                return;
            }
            if (input == StatisticVersionArchivalStatus::StatisticVersionArchivalStatusQueued)
            {
                output = Json::Value("Queued");
                return;
            }
            if (input == StatisticVersionArchivalStatus::StatisticVersionArchivalStatusInProgress)
            {
                output = Json::Value("InProgress");
                return;
            }
            if (input == StatisticVersionArchivalStatus::StatisticVersionArchivalStatusComplete)
            {
                output = Json::Value("Complete");
                return;
            }
        }
        inline void FromJsonEnum(const Json::Value& input, StatisticVersionArchivalStatus& output)
        {
            if (!input.isString())
            {
                return;
            }
            const std::string& inputStr = input.asString();
            if (inputStr == "NotScheduled")
            {
                output = StatisticVersionArchivalStatus::StatisticVersionArchivalStatusNotScheduled;
                return;
            }
            if (inputStr == "Scheduled")
            {
                output = StatisticVersionArchivalStatus::StatisticVersionArchivalStatusScheduled;
                return;
            }
            if (inputStr == "Queued")
            {
                output = StatisticVersionArchivalStatus::StatisticVersionArchivalStatusQueued;
                return;
            }
            if (inputStr == "InProgress")
            {
                output = StatisticVersionArchivalStatus::StatisticVersionArchivalStatusInProgress;
                return;
            }
            if (inputStr == "Complete")
            {
                output = StatisticVersionArchivalStatus::StatisticVersionArchivalStatusComplete;
                return;
            }
        }

        enum class StatisticVersionStatus
        {
            StatisticVersionStatusActive,
            StatisticVersionStatusSnapshotPending,
            StatisticVersionStatusSnapshot,
            StatisticVersionStatusArchivalPending,
            StatisticVersionStatusArchived
        };

        inline void ToJsonEnum(const StatisticVersionStatus input, Json::Value& output)
        {
            if (input == StatisticVersionStatus::StatisticVersionStatusActive)
            {
                output = Json::Value("Active");
                return;
            }
            if (input == StatisticVersionStatus::StatisticVersionStatusSnapshotPending)
            {
                output = Json::Value("SnapshotPending");
                return;
            }
            if (input == StatisticVersionStatus::StatisticVersionStatusSnapshot)
            {
                output = Json::Value("Snapshot");
                return;
            }
            if (input == StatisticVersionStatus::StatisticVersionStatusArchivalPending)
            {
                output = Json::Value("ArchivalPending");
                return;
            }
            if (input == StatisticVersionStatus::StatisticVersionStatusArchived)
            {
                output = Json::Value("Archived");
                return;
            }
        }
        inline void FromJsonEnum(const Json::Value& input, StatisticVersionStatus& output)
        {
            if (!input.isString())
            {
                return;
            }
            const std::string& inputStr = input.asString();
            if (inputStr == "Active")
            {
                output = StatisticVersionStatus::StatisticVersionStatusActive;
                return;
            }
            if (inputStr == "SnapshotPending")
            {
                output = StatisticVersionStatus::StatisticVersionStatusSnapshotPending;
                return;
            }
            if (inputStr == "Snapshot")
            {
                output = StatisticVersionStatus::StatisticVersionStatusSnapshot;
                return;
            }
            if (inputStr == "ArchivalPending")
            {
                output = StatisticVersionStatus::StatisticVersionStatusArchivalPending;
                return;
            }
            if (inputStr == "Archived")
            {
                output = StatisticVersionStatus::StatisticVersionStatusArchived;
                return;
            }
        }

        enum class SubscriptionProviderStatus
        {
            SubscriptionProviderStatusNoError,
            SubscriptionProviderStatusCancelled,
            SubscriptionProviderStatusUnknownError,
            SubscriptionProviderStatusBillingError,
            SubscriptionProviderStatusProductUnavailable,
            SubscriptionProviderStatusCustomerDidNotAcceptPriceChange,
            SubscriptionProviderStatusFreeTrial,
            SubscriptionProviderStatusPaymentPending
        };

        inline void ToJsonEnum(const SubscriptionProviderStatus input, Json::Value& output)
        {
            if (input == SubscriptionProviderStatus::SubscriptionProviderStatusNoError)
            {
                output = Json::Value("NoError");
                return;
            }
            if (input == SubscriptionProviderStatus::SubscriptionProviderStatusCancelled)
            {
                output = Json::Value("Cancelled");
                return;
            }
            if (input == SubscriptionProviderStatus::SubscriptionProviderStatusUnknownError)
            {
                output = Json::Value("UnknownError");
                return;
            }
            if (input == SubscriptionProviderStatus::SubscriptionProviderStatusBillingError)
            {
                output = Json::Value("BillingError");
                return;
            }
            if (input == SubscriptionProviderStatus::SubscriptionProviderStatusProductUnavailable)
            {
                output = Json::Value("ProductUnavailable");
                return;
            }
            if (input == SubscriptionProviderStatus::SubscriptionProviderStatusCustomerDidNotAcceptPriceChange)
            {
                output = Json::Value("CustomerDidNotAcceptPriceChange");
                return;
            }
            if (input == SubscriptionProviderStatus::SubscriptionProviderStatusFreeTrial)
            {
                output = Json::Value("FreeTrial");
                return;
            }
            if (input == SubscriptionProviderStatus::SubscriptionProviderStatusPaymentPending)
            {
                output = Json::Value("PaymentPending");
                return;
            }
        }
        inline void FromJsonEnum(const Json::Value& input, SubscriptionProviderStatus& output)
        {
            if (!input.isString())
            {
                return;
            }
            const std::string& inputStr = input.asString();
            if (inputStr == "NoError")
            {
                output = SubscriptionProviderStatus::SubscriptionProviderStatusNoError;
                return;
            }
            if (inputStr == "Cancelled")
            {
                output = SubscriptionProviderStatus::SubscriptionProviderStatusCancelled;
                return;
            }
            if (inputStr == "UnknownError")
            {
                output = SubscriptionProviderStatus::SubscriptionProviderStatusUnknownError;
                return;
            }
            if (inputStr == "BillingError")
            {
                output = SubscriptionProviderStatus::SubscriptionProviderStatusBillingError;
                return;
            }
            if (inputStr == "ProductUnavailable")
            {
                output = SubscriptionProviderStatus::SubscriptionProviderStatusProductUnavailable;
                return;
            }
            if (inputStr == "CustomerDidNotAcceptPriceChange")
            {
                output = SubscriptionProviderStatus::SubscriptionProviderStatusCustomerDidNotAcceptPriceChange;
                return;
            }
            if (inputStr == "FreeTrial")
            {
                output = SubscriptionProviderStatus::SubscriptionProviderStatusFreeTrial;
                return;
            }
            if (inputStr == "PaymentPending")
            {
                output = SubscriptionProviderStatus::SubscriptionProviderStatusPaymentPending;
                return;
            }
        }

        enum class TaskInstanceStatus
        {
            TaskInstanceStatusSucceeded,
            TaskInstanceStatusStarting,
            TaskInstanceStatusInProgress,
            TaskInstanceStatusFailed,
            TaskInstanceStatusAborted,
            TaskInstanceStatusStalled
        };

        inline void ToJsonEnum(const TaskInstanceStatus input, Json::Value& output)
        {
            if (input == TaskInstanceStatus::TaskInstanceStatusSucceeded)
            {
                output = Json::Value("Succeeded");
                return;
            }
            if (input == TaskInstanceStatus::TaskInstanceStatusStarting)
            {
                output = Json::Value("Starting");
                return;
            }
            if (input == TaskInstanceStatus::TaskInstanceStatusInProgress)
            {
                output = Json::Value("InProgress");
                return;
            }
            if (input == TaskInstanceStatus::TaskInstanceStatusFailed)
            {
                output = Json::Value("Failed");
                return;
            }
            if (input == TaskInstanceStatus::TaskInstanceStatusAborted)
            {
                output = Json::Value("Aborted");
                return;
            }
            if (input == TaskInstanceStatus::TaskInstanceStatusStalled)
            {
                output = Json::Value("Stalled");
                return;
            }
        }
        inline void FromJsonEnum(const Json::Value& input, TaskInstanceStatus& output)
        {
            if (!input.isString())
            {
                return;
            }
            const std::string& inputStr = input.asString();
            if (inputStr == "Succeeded")
            {
                output = TaskInstanceStatus::TaskInstanceStatusSucceeded;
                return;
            }
            if (inputStr == "Starting")
            {
                output = TaskInstanceStatus::TaskInstanceStatusStarting;
                return;
            }
            if (inputStr == "InProgress")
            {
                output = TaskInstanceStatus::TaskInstanceStatusInProgress;
                return;
            }
            if (inputStr == "Failed")
            {
                output = TaskInstanceStatus::TaskInstanceStatusFailed;
                return;
            }
            if (inputStr == "Aborted")
            {
                output = TaskInstanceStatus::TaskInstanceStatusAborted;
                return;
            }
            if (inputStr == "Stalled")
            {
                output = TaskInstanceStatus::TaskInstanceStatusStalled;
                return;
            }
        }

        enum class TitleActivationStatus
        {
            TitleActivationStatusNone,
            TitleActivationStatusActivatedTitleKey,
            TitleActivationStatusPendingSteam,
            TitleActivationStatusActivatedSteam,
            TitleActivationStatusRevokedSteam
        };

        inline void ToJsonEnum(const TitleActivationStatus input, Json::Value& output)
        {
            if (input == TitleActivationStatus::TitleActivationStatusNone)
            {
                output = Json::Value("None");
                return;
            }
            if (input == TitleActivationStatus::TitleActivationStatusActivatedTitleKey)
            {
                output = Json::Value("ActivatedTitleKey");
                return;
            }
            if (input == TitleActivationStatus::TitleActivationStatusPendingSteam)
            {
                output = Json::Value("PendingSteam");
                return;
            }
            if (input == TitleActivationStatus::TitleActivationStatusActivatedSteam)
            {
                output = Json::Value("ActivatedSteam");
                return;
            }
            if (input == TitleActivationStatus::TitleActivationStatusRevokedSteam)
            {
                output = Json::Value("RevokedSteam");
                return;
            }
        }
        inline void FromJsonEnum(const Json::Value& input, TitleActivationStatus& output)
        {
            if (!input.isString())
            {
                return;
            }
            const std::string& inputStr = input.asString();
            if (inputStr == "None")
            {
                output = TitleActivationStatus::TitleActivationStatusNone;
                return;
            }
            if (inputStr == "ActivatedTitleKey")
            {
                output = TitleActivationStatus::TitleActivationStatusActivatedTitleKey;
                return;
            }
            if (inputStr == "PendingSteam")
            {
                output = TitleActivationStatus::TitleActivationStatusPendingSteam;
                return;
            }
            if (inputStr == "ActivatedSteam")
            {
                output = TitleActivationStatus::TitleActivationStatusActivatedSteam;
                return;
            }
            if (inputStr == "RevokedSteam")
            {
                output = TitleActivationStatus::TitleActivationStatusRevokedSteam;
                return;
            }
        }

        enum class UserDataPermission
        {
            UserDataPermissionPrivate,
            UserDataPermissionPublic
        };

        inline void ToJsonEnum(const UserDataPermission input, Json::Value& output)
        {
            if (input == UserDataPermission::UserDataPermissionPrivate)
            {
                output = Json::Value("Private");
                return;
            }
            if (input == UserDataPermission::UserDataPermissionPublic)
            {
                output = Json::Value("Public");
                return;
            }
        }
        inline void FromJsonEnum(const Json::Value& input, UserDataPermission& output)
        {
            if (!input.isString())
            {
                return;
            }
            const std::string& inputStr = input.asString();
            if (inputStr == "Private")
            {
                output = UserDataPermission::UserDataPermissionPrivate;
                return;
            }
            if (inputStr == "Public")
            {
                output = UserDataPermission::UserDataPermissionPublic;
                return;
            }
        }

        enum class UserOrigination
        {
            UserOriginationOrganic,
            UserOriginationSteam,
            UserOriginationGoogle,
            UserOriginationAmazon,
            UserOriginationFacebook,
            UserOriginationKongregate,
            UserOriginationGamersFirst,
            UserOriginationUnknown,
            UserOriginationIOS,
            UserOriginationLoadTest,
            UserOriginationAndroid,
            UserOriginationPSN,
            UserOriginationGameCenter,
            UserOriginationCustomId,
            UserOriginationXboxLive,
            UserOriginationParse,
            UserOriginationTwitch,
            UserOriginationWindowsHello,
            UserOriginationServerCustomId,
            UserOriginationNintendoSwitchDeviceId,
            UserOriginationFacebookInstantGamesId,
            UserOriginationOpenIdConnect,
            UserOriginationApple,
            UserOriginationNintendoSwitchAccount
        };

        inline void ToJsonEnum(const UserOrigination input, Json::Value& output)
        {
            if (input == UserOrigination::UserOriginationOrganic)
            {
                output = Json::Value("Organic");
                return;
            }
            if (input == UserOrigination::UserOriginationSteam)
            {
                output = Json::Value("Steam");
                return;
            }
            if (input == UserOrigination::UserOriginationGoogle)
            {
                output = Json::Value("Google");
                return;
            }
            if (input == UserOrigination::UserOriginationAmazon)
            {
                output = Json::Value("Amazon");
                return;
            }
            if (input == UserOrigination::UserOriginationFacebook)
            {
                output = Json::Value("Facebook");
                return;
            }
            if (input == UserOrigination::UserOriginationKongregate)
            {
                output = Json::Value("Kongregate");
                return;
            }
            if (input == UserOrigination::UserOriginationGamersFirst)
            {
                output = Json::Value("GamersFirst");
                return;
            }
            if (input == UserOrigination::UserOriginationUnknown)
            {
                output = Json::Value("Unknown");
                return;
            }
            if (input == UserOrigination::UserOriginationIOS)
            {
                output = Json::Value("IOS");
                return;
            }
            if (input == UserOrigination::UserOriginationLoadTest)
            {
                output = Json::Value("LoadTest");
                return;
            }
            if (input == UserOrigination::UserOriginationAndroid)
            {
                output = Json::Value("Android");
                return;
            }
            if (input == UserOrigination::UserOriginationPSN)
            {
                output = Json::Value("PSN");
                return;
            }
            if (input == UserOrigination::UserOriginationGameCenter)
            {
                output = Json::Value("GameCenter");
                return;
            }
            if (input == UserOrigination::UserOriginationCustomId)
            {
                output = Json::Value("CustomId");
                return;
            }
            if (input == UserOrigination::UserOriginationXboxLive)
            {
                output = Json::Value("XboxLive");
                return;
            }
            if (input == UserOrigination::UserOriginationParse)
            {
                output = Json::Value("Parse");
                return;
            }
            if (input == UserOrigination::UserOriginationTwitch)
            {
                output = Json::Value("Twitch");
                return;
            }
            if (input == UserOrigination::UserOriginationWindowsHello)
            {
                output = Json::Value("WindowsHello");
                return;
            }
            if (input == UserOrigination::UserOriginationServerCustomId)
            {
                output = Json::Value("ServerCustomId");
                return;
            }
            if (input == UserOrigination::UserOriginationNintendoSwitchDeviceId)
            {
                output = Json::Value("NintendoSwitchDeviceId");
                return;
            }
            if (input == UserOrigination::UserOriginationFacebookInstantGamesId)
            {
                output = Json::Value("FacebookInstantGamesId");
                return;
            }
            if (input == UserOrigination::UserOriginationOpenIdConnect)
            {
                output = Json::Value("OpenIdConnect");
                return;
            }
            if (input == UserOrigination::UserOriginationApple)
            {
                output = Json::Value("Apple");
                return;
            }
            if (input == UserOrigination::UserOriginationNintendoSwitchAccount)
            {
                output = Json::Value("NintendoSwitchAccount");
                return;
            }
        }
        inline void FromJsonEnum(const Json::Value& input, UserOrigination& output)
        {
            if (!input.isString())
            {
                return;
            }
            const std::string& inputStr = input.asString();
            if (inputStr == "Organic")
            {
                output = UserOrigination::UserOriginationOrganic;
                return;
            }
            if (inputStr == "Steam")
            {
                output = UserOrigination::UserOriginationSteam;
                return;
            }
            if (inputStr == "Google")
            {
                output = UserOrigination::UserOriginationGoogle;
                return;
            }
            if (inputStr == "Amazon")
            {
                output = UserOrigination::UserOriginationAmazon;
                return;
            }
            if (inputStr == "Facebook")
            {
                output = UserOrigination::UserOriginationFacebook;
                return;
            }
            if (inputStr == "Kongregate")
            {
                output = UserOrigination::UserOriginationKongregate;
                return;
            }
            if (inputStr == "GamersFirst")
            {
                output = UserOrigination::UserOriginationGamersFirst;
                return;
            }
            if (inputStr == "Unknown")
            {
                output = UserOrigination::UserOriginationUnknown;
                return;
            }
            if (inputStr == "IOS")
            {
                output = UserOrigination::UserOriginationIOS;
                return;
            }
            if (inputStr == "LoadTest")
            {
                output = UserOrigination::UserOriginationLoadTest;
                return;
            }
            if (inputStr == "Android")
            {
                output = UserOrigination::UserOriginationAndroid;
                return;
            }
            if (inputStr == "PSN")
            {
                output = UserOrigination::UserOriginationPSN;
                return;
            }
            if (inputStr == "GameCenter")
            {
                output = UserOrigination::UserOriginationGameCenter;
                return;
            }
            if (inputStr == "CustomId")
            {
                output = UserOrigination::UserOriginationCustomId;
                return;
            }
            if (inputStr == "XboxLive")
            {
                output = UserOrigination::UserOriginationXboxLive;
                return;
            }
            if (inputStr == "Parse")
            {
                output = UserOrigination::UserOriginationParse;
                return;
            }
            if (inputStr == "Twitch")
            {
                output = UserOrigination::UserOriginationTwitch;
                return;
            }
            if (inputStr == "WindowsHello")
            {
                output = UserOrigination::UserOriginationWindowsHello;
                return;
            }
            if (inputStr == "ServerCustomId")
            {
                output = UserOrigination::UserOriginationServerCustomId;
                return;
            }
            if (inputStr == "NintendoSwitchDeviceId")
            {
                output = UserOrigination::UserOriginationNintendoSwitchDeviceId;
                return;
            }
            if (inputStr == "FacebookInstantGamesId")
            {
                output = UserOrigination::UserOriginationFacebookInstantGamesId;
                return;
            }
            if (inputStr == "OpenIdConnect")
            {
                output = UserOrigination::UserOriginationOpenIdConnect;
                return;
            }
            if (inputStr == "Apple")
            {
                output = UserOrigination::UserOriginationApple;
                return;
            }
            if (inputStr == "NintendoSwitchAccount")
            {
                output = UserOrigination::UserOriginationNintendoSwitchAccount;
                return;
            }
        }

        // Admin Classes
        struct AbortTaskInstanceRequest : public PlayFabRequestCommon
        {
            std::map<std::string, std::string> CustomTags;
            std::string TaskInstanceId;

            AbortTaskInstanceRequest() :
                PlayFabRequestCommon(),
                CustomTags(),
                TaskInstanceId()
            {}

            AbortTaskInstanceRequest(const AbortTaskInstanceRequest& src) :
                PlayFabRequestCommon(),
                CustomTags(src.CustomTags),
                TaskInstanceId(src.TaskInstanceId)
            {}

            ~AbortTaskInstanceRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["CustomTags"], CustomTags);
                FromJsonUtilS(input["TaskInstanceId"], TaskInstanceId);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_CustomTags; ToJsonUtilS(CustomTags, each_CustomTags); output["CustomTags"] = each_CustomTags;
                Json::Value each_TaskInstanceId; ToJsonUtilS(TaskInstanceId, each_TaskInstanceId); output["TaskInstanceId"] = each_TaskInstanceId;
                return output;
            }
        };

        struct ActionsOnPlayersInSegmentTaskParameter : public PlayFabBaseModel
        {
            std::string ActionId;
            std::string SegmentId;

            ActionsOnPlayersInSegmentTaskParameter() :
                PlayFabBaseModel(),
                ActionId(),
                SegmentId()
            {}

            ActionsOnPlayersInSegmentTaskParameter(const ActionsOnPlayersInSegmentTaskParameter& src) :
                PlayFabBaseModel(),
                ActionId(src.ActionId),
                SegmentId(src.SegmentId)
            {}

            ~ActionsOnPlayersInSegmentTaskParameter() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["ActionId"], ActionId);
                FromJsonUtilS(input["SegmentId"], SegmentId);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_ActionId; ToJsonUtilS(ActionId, each_ActionId); output["ActionId"] = each_ActionId;
                Json::Value each_SegmentId; ToJsonUtilS(SegmentId, each_SegmentId); output["SegmentId"] = each_SegmentId;
                return output;
            }
        };

        struct NameIdentifier : public PlayFabBaseModel
        {
            std::string Id;
            std::string Name;

            NameIdentifier() :
                PlayFabBaseModel(),
                Id(),
                Name()
            {}

            NameIdentifier(const NameIdentifier& src) :
                PlayFabBaseModel(),
                Id(src.Id),
                Name(src.Name)
            {}

            ~NameIdentifier() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["Id"], Id);
                FromJsonUtilS(input["Name"], Name);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_Id; ToJsonUtilS(Id, each_Id); output["Id"] = each_Id;
                Json::Value each_Name; ToJsonUtilS(Name, each_Name); output["Name"] = each_Name;
                return output;
            }
        };

        struct ActionsOnPlayersInSegmentTaskSummary : public PlayFabBaseModel
        {
            Boxed<time_t> CompletedAt;
            std::string ErrorMessage;
            Boxed<bool> ErrorWasFatal;
            Boxed<double> EstimatedSecondsRemaining;
            Boxed<double> PercentComplete;
            std::string ScheduledByUserId;
            time_t StartedAt;
            Boxed<TaskInstanceStatus> Status;
            Boxed<NameIdentifier> TaskIdentifier;
            std::string TaskInstanceId;
            Boxed<Int32> TotalPlayersInSegment;
            Boxed<Int32> TotalPlayersProcessed;

            ActionsOnPlayersInSegmentTaskSummary() :
                PlayFabBaseModel(),
                CompletedAt(),
                ErrorMessage(),
                ErrorWasFatal(),
                EstimatedSecondsRemaining(),
                PercentComplete(),
                ScheduledByUserId(),
                StartedAt(),
                Status(),
                TaskIdentifier(),
                TaskInstanceId(),
                TotalPlayersInSegment(),
                TotalPlayersProcessed()
            {}

            ActionsOnPlayersInSegmentTaskSummary(const ActionsOnPlayersInSegmentTaskSummary& src) :
                PlayFabBaseModel(),
                CompletedAt(src.CompletedAt),
                ErrorMessage(src.ErrorMessage),
                ErrorWasFatal(src.ErrorWasFatal),
                EstimatedSecondsRemaining(src.EstimatedSecondsRemaining),
                PercentComplete(src.PercentComplete),
                ScheduledByUserId(src.ScheduledByUserId),
                StartedAt(src.StartedAt),
                Status(src.Status),
                TaskIdentifier(src.TaskIdentifier),
                TaskInstanceId(src.TaskInstanceId),
                TotalPlayersInSegment(src.TotalPlayersInSegment),
                TotalPlayersProcessed(src.TotalPlayersProcessed)
            {}

            ~ActionsOnPlayersInSegmentTaskSummary() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilT(input["CompletedAt"], CompletedAt);
                FromJsonUtilS(input["ErrorMessage"], ErrorMessage);
                FromJsonUtilP(input["ErrorWasFatal"], ErrorWasFatal);
                FromJsonUtilP(input["EstimatedSecondsRemaining"], EstimatedSecondsRemaining);
                FromJsonUtilP(input["PercentComplete"], PercentComplete);
                FromJsonUtilS(input["ScheduledByUserId"], ScheduledByUserId);
                FromJsonUtilT(input["StartedAt"], StartedAt);
                FromJsonUtilE(input["Status"], Status);
                FromJsonUtilO(input["TaskIdentifier"], TaskIdentifier);
                FromJsonUtilS(input["TaskInstanceId"], TaskInstanceId);
                FromJsonUtilP(input["TotalPlayersInSegment"], TotalPlayersInSegment);
                FromJsonUtilP(input["TotalPlayersProcessed"], TotalPlayersProcessed);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_CompletedAt; ToJsonUtilT(CompletedAt, each_CompletedAt); output["CompletedAt"] = each_CompletedAt;
                Json::Value each_ErrorMessage; ToJsonUtilS(ErrorMessage, each_ErrorMessage); output["ErrorMessage"] = each_ErrorMessage;
                Json::Value each_ErrorWasFatal; ToJsonUtilP(ErrorWasFatal, each_ErrorWasFatal); output["ErrorWasFatal"] = each_ErrorWasFatal;
                Json::Value each_EstimatedSecondsRemaining; ToJsonUtilP(EstimatedSecondsRemaining, each_EstimatedSecondsRemaining); output["EstimatedSecondsRemaining"] = each_EstimatedSecondsRemaining;
                Json::Value each_PercentComplete; ToJsonUtilP(PercentComplete, each_PercentComplete); output["PercentComplete"] = each_PercentComplete;
                Json::Value each_ScheduledByUserId; ToJsonUtilS(ScheduledByUserId, each_ScheduledByUserId); output["ScheduledByUserId"] = each_ScheduledByUserId;
                Json::Value each_StartedAt; ToJsonUtilT(StartedAt, each_StartedAt); output["StartedAt"] = each_StartedAt;
                Json::Value each_Status; ToJsonUtilE(Status, each_Status); output["Status"] = each_Status;
                Json::Value each_TaskIdentifier; ToJsonUtilO(TaskIdentifier, each_TaskIdentifier); output["TaskIdentifier"] = each_TaskIdentifier;
                Json::Value each_TaskInstanceId; ToJsonUtilS(TaskInstanceId, each_TaskInstanceId); output["TaskInstanceId"] = each_TaskInstanceId;
                Json::Value each_TotalPlayersInSegment; ToJsonUtilP(TotalPlayersInSegment, each_TotalPlayersInSegment); output["TotalPlayersInSegment"] = each_TotalPlayersInSegment;
                Json::Value each_TotalPlayersProcessed; ToJsonUtilP(TotalPlayersProcessed, each_TotalPlayersProcessed); output["TotalPlayersProcessed"] = each_TotalPlayersProcessed;
                return output;
            }
        };

        struct AdCampaignAttribution : public PlayFabBaseModel
        {
            time_t AttributedAt;
            std::string CampaignId;
            std::string Platform;

            AdCampaignAttribution() :
                PlayFabBaseModel(),
                AttributedAt(),
                CampaignId(),
                Platform()
            {}

            AdCampaignAttribution(const AdCampaignAttribution& src) :
                PlayFabBaseModel(),
                AttributedAt(src.AttributedAt),
                CampaignId(src.CampaignId),
                Platform(src.Platform)
            {}

            ~AdCampaignAttribution() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilT(input["AttributedAt"], AttributedAt);
                FromJsonUtilS(input["CampaignId"], CampaignId);
                FromJsonUtilS(input["Platform"], Platform);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_AttributedAt; ToJsonUtilT(AttributedAt, each_AttributedAt); output["AttributedAt"] = each_AttributedAt;
                Json::Value each_CampaignId; ToJsonUtilS(CampaignId, each_CampaignId); output["CampaignId"] = each_CampaignId;
                Json::Value each_Platform; ToJsonUtilS(Platform, each_Platform); output["Platform"] = each_Platform;
                return output;
            }
        };

        struct AdCampaignAttributionModel : public PlayFabBaseModel
        {
            time_t AttributedAt;
            std::string CampaignId;
            std::string Platform;

            AdCampaignAttributionModel() :
                PlayFabBaseModel(),
                AttributedAt(),
                CampaignId(),
                Platform()
            {}

            AdCampaignAttributionModel(const AdCampaignAttributionModel& src) :
                PlayFabBaseModel(),
                AttributedAt(src.AttributedAt),
                CampaignId(src.CampaignId),
                Platform(src.Platform)
            {}

            ~AdCampaignAttributionModel() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilT(input["AttributedAt"], AttributedAt);
                FromJsonUtilS(input["CampaignId"], CampaignId);
                FromJsonUtilS(input["Platform"], Platform);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_AttributedAt; ToJsonUtilT(AttributedAt, each_AttributedAt); output["AttributedAt"] = each_AttributedAt;
                Json::Value each_CampaignId; ToJsonUtilS(CampaignId, each_CampaignId); output["CampaignId"] = each_CampaignId;
                Json::Value each_Platform; ToJsonUtilS(Platform, each_Platform); output["Platform"] = each_Platform;
                return output;
            }
        };

        struct AddLocalizedNewsRequest : public PlayFabRequestCommon
        {
            std::string Body;
            std::map<std::string, std::string> CustomTags;
            std::string Language;
            std::string NewsId;
            std::string Title;

            AddLocalizedNewsRequest() :
                PlayFabRequestCommon(),
                Body(),
                CustomTags(),
                Language(),
                NewsId(),
                Title()
            {}

            AddLocalizedNewsRequest(const AddLocalizedNewsRequest& src) :
                PlayFabRequestCommon(),
                Body(src.Body),
                CustomTags(src.CustomTags),
                Language(src.Language),
                NewsId(src.NewsId),
                Title(src.Title)
            {}

            ~AddLocalizedNewsRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["Body"], Body);
                FromJsonUtilS(input["CustomTags"], CustomTags);
                FromJsonUtilS(input["Language"], Language);
                FromJsonUtilS(input["NewsId"], NewsId);
                FromJsonUtilS(input["Title"], Title);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_Body; ToJsonUtilS(Body, each_Body); output["Body"] = each_Body;
                Json::Value each_CustomTags; ToJsonUtilS(CustomTags, each_CustomTags); output["CustomTags"] = each_CustomTags;
                Json::Value each_Language; ToJsonUtilS(Language, each_Language); output["Language"] = each_Language;
                Json::Value each_NewsId; ToJsonUtilS(NewsId, each_NewsId); output["NewsId"] = each_NewsId;
                Json::Value each_Title; ToJsonUtilS(Title, each_Title); output["Title"] = each_Title;
                return output;
            }
        };

        struct AddLocalizedNewsResult : public PlayFabResultCommon
        {

            AddLocalizedNewsResult() :
                PlayFabResultCommon()
            {}

            AddLocalizedNewsResult(const AddLocalizedNewsResult&) :
                PlayFabResultCommon()
            {}

            ~AddLocalizedNewsResult() = default;

            void FromJson(const Json::Value&) override
            {
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                return output;
            }
        };

        struct AddNewsRequest : public PlayFabRequestCommon
        {
            std::string Body;
            std::map<std::string, std::string> CustomTags;
            Boxed<time_t> Timestamp;
            std::string Title;

            AddNewsRequest() :
                PlayFabRequestCommon(),
                Body(),
                CustomTags(),
                Timestamp(),
                Title()
            {}

            AddNewsRequest(const AddNewsRequest& src) :
                PlayFabRequestCommon(),
                Body(src.Body),
                CustomTags(src.CustomTags),
                Timestamp(src.Timestamp),
                Title(src.Title)
            {}

            ~AddNewsRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["Body"], Body);
                FromJsonUtilS(input["CustomTags"], CustomTags);
                FromJsonUtilT(input["Timestamp"], Timestamp);
                FromJsonUtilS(input["Title"], Title);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_Body; ToJsonUtilS(Body, each_Body); output["Body"] = each_Body;
                Json::Value each_CustomTags; ToJsonUtilS(CustomTags, each_CustomTags); output["CustomTags"] = each_CustomTags;
                Json::Value each_Timestamp; ToJsonUtilT(Timestamp, each_Timestamp); output["Timestamp"] = each_Timestamp;
                Json::Value each_Title; ToJsonUtilS(Title, each_Title); output["Title"] = each_Title;
                return output;
            }
        };

        struct AddNewsResult : public PlayFabResultCommon
        {
            std::string NewsId;

            AddNewsResult() :
                PlayFabResultCommon(),
                NewsId()
            {}

            AddNewsResult(const AddNewsResult& src) :
                PlayFabResultCommon(),
                NewsId(src.NewsId)
            {}

            ~AddNewsResult() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["NewsId"], NewsId);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_NewsId; ToJsonUtilS(NewsId, each_NewsId); output["NewsId"] = each_NewsId;
                return output;
            }
        };

        struct AddPlayerTagRequest : public PlayFabRequestCommon
        {
            std::map<std::string, std::string> CustomTags;
            std::string PlayFabId;
            std::string TagName;

            AddPlayerTagRequest() :
                PlayFabRequestCommon(),
                CustomTags(),
                PlayFabId(),
                TagName()
            {}

            AddPlayerTagRequest(const AddPlayerTagRequest& src) :
                PlayFabRequestCommon(),
                CustomTags(src.CustomTags),
                PlayFabId(src.PlayFabId),
                TagName(src.TagName)
            {}

            ~AddPlayerTagRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["CustomTags"], CustomTags);
                FromJsonUtilS(input["PlayFabId"], PlayFabId);
                FromJsonUtilS(input["TagName"], TagName);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_CustomTags; ToJsonUtilS(CustomTags, each_CustomTags); output["CustomTags"] = each_CustomTags;
                Json::Value each_PlayFabId; ToJsonUtilS(PlayFabId, each_PlayFabId); output["PlayFabId"] = each_PlayFabId;
                Json::Value each_TagName; ToJsonUtilS(TagName, each_TagName); output["TagName"] = each_TagName;
                return output;
            }
        };

        struct AddPlayerTagResult : public PlayFabResultCommon
        {

            AddPlayerTagResult() :
                PlayFabResultCommon()
            {}

            AddPlayerTagResult(const AddPlayerTagResult&) :
                PlayFabResultCommon()
            {}

            ~AddPlayerTagResult() = default;

            void FromJson(const Json::Value&) override
            {
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                return output;
            }
        };

        struct AddServerBuildRequest : public PlayFabRequestCommon
        {
            std::list<Region> ActiveRegions;
            std::string BuildId;
            std::string CommandLineTemplate;
            std::string Comment;
            std::map<std::string, std::string> CustomTags;
            std::string ExecutablePath;
            Int32 MaxGamesPerHost;
            Int32 MinFreeGameSlots;

            AddServerBuildRequest() :
                PlayFabRequestCommon(),
                ActiveRegions(),
                BuildId(),
                CommandLineTemplate(),
                Comment(),
                CustomTags(),
                ExecutablePath(),
                MaxGamesPerHost(),
                MinFreeGameSlots()
            {}

            AddServerBuildRequest(const AddServerBuildRequest& src) :
                PlayFabRequestCommon(),
                ActiveRegions(src.ActiveRegions),
                BuildId(src.BuildId),
                CommandLineTemplate(src.CommandLineTemplate),
                Comment(src.Comment),
                CustomTags(src.CustomTags),
                ExecutablePath(src.ExecutablePath),
                MaxGamesPerHost(src.MaxGamesPerHost),
                MinFreeGameSlots(src.MinFreeGameSlots)
            {}

            ~AddServerBuildRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilE(input["ActiveRegions"], ActiveRegions);
                FromJsonUtilS(input["BuildId"], BuildId);
                FromJsonUtilS(input["CommandLineTemplate"], CommandLineTemplate);
                FromJsonUtilS(input["Comment"], Comment);
                FromJsonUtilS(input["CustomTags"], CustomTags);
                FromJsonUtilS(input["ExecutablePath"], ExecutablePath);
                FromJsonUtilP(input["MaxGamesPerHost"], MaxGamesPerHost);
                FromJsonUtilP(input["MinFreeGameSlots"], MinFreeGameSlots);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_ActiveRegions; ToJsonUtilE(ActiveRegions, each_ActiveRegions); output["ActiveRegions"] = each_ActiveRegions;
                Json::Value each_BuildId; ToJsonUtilS(BuildId, each_BuildId); output["BuildId"] = each_BuildId;
                Json::Value each_CommandLineTemplate; ToJsonUtilS(CommandLineTemplate, each_CommandLineTemplate); output["CommandLineTemplate"] = each_CommandLineTemplate;
                Json::Value each_Comment; ToJsonUtilS(Comment, each_Comment); output["Comment"] = each_Comment;
                Json::Value each_CustomTags; ToJsonUtilS(CustomTags, each_CustomTags); output["CustomTags"] = each_CustomTags;
                Json::Value each_ExecutablePath; ToJsonUtilS(ExecutablePath, each_ExecutablePath); output["ExecutablePath"] = each_ExecutablePath;
                Json::Value each_MaxGamesPerHost; ToJsonUtilP(MaxGamesPerHost, each_MaxGamesPerHost); output["MaxGamesPerHost"] = each_MaxGamesPerHost;
                Json::Value each_MinFreeGameSlots; ToJsonUtilP(MinFreeGameSlots, each_MinFreeGameSlots); output["MinFreeGameSlots"] = each_MinFreeGameSlots;
                return output;
            }
        };

        struct AddServerBuildResult : public PlayFabResultCommon
        {
            std::list<Region> ActiveRegions;
            std::string BuildId;
            std::string CommandLineTemplate;
            std::string Comment;
            std::string ExecutablePath;
            Int32 MaxGamesPerHost;
            Int32 MinFreeGameSlots;
            Boxed<GameBuildStatus> Status;
            time_t Timestamp;
            std::string TitleId;

            AddServerBuildResult() :
                PlayFabResultCommon(),
                ActiveRegions(),
                BuildId(),
                CommandLineTemplate(),
                Comment(),
                ExecutablePath(),
                MaxGamesPerHost(),
                MinFreeGameSlots(),
                Status(),
                Timestamp(),
                TitleId()
            {}

            AddServerBuildResult(const AddServerBuildResult& src) :
                PlayFabResultCommon(),
                ActiveRegions(src.ActiveRegions),
                BuildId(src.BuildId),
                CommandLineTemplate(src.CommandLineTemplate),
                Comment(src.Comment),
                ExecutablePath(src.ExecutablePath),
                MaxGamesPerHost(src.MaxGamesPerHost),
                MinFreeGameSlots(src.MinFreeGameSlots),
                Status(src.Status),
                Timestamp(src.Timestamp),
                TitleId(src.TitleId)
            {}

            ~AddServerBuildResult() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilE(input["ActiveRegions"], ActiveRegions);
                FromJsonUtilS(input["BuildId"], BuildId);
                FromJsonUtilS(input["CommandLineTemplate"], CommandLineTemplate);
                FromJsonUtilS(input["Comment"], Comment);
                FromJsonUtilS(input["ExecutablePath"], ExecutablePath);
                FromJsonUtilP(input["MaxGamesPerHost"], MaxGamesPerHost);
                FromJsonUtilP(input["MinFreeGameSlots"], MinFreeGameSlots);
                FromJsonUtilE(input["Status"], Status);
                FromJsonUtilT(input["Timestamp"], Timestamp);
                FromJsonUtilS(input["TitleId"], TitleId);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_ActiveRegions; ToJsonUtilE(ActiveRegions, each_ActiveRegions); output["ActiveRegions"] = each_ActiveRegions;
                Json::Value each_BuildId; ToJsonUtilS(BuildId, each_BuildId); output["BuildId"] = each_BuildId;
                Json::Value each_CommandLineTemplate; ToJsonUtilS(CommandLineTemplate, each_CommandLineTemplate); output["CommandLineTemplate"] = each_CommandLineTemplate;
                Json::Value each_Comment; ToJsonUtilS(Comment, each_Comment); output["Comment"] = each_Comment;
                Json::Value each_ExecutablePath; ToJsonUtilS(ExecutablePath, each_ExecutablePath); output["ExecutablePath"] = each_ExecutablePath;
                Json::Value each_MaxGamesPerHost; ToJsonUtilP(MaxGamesPerHost, each_MaxGamesPerHost); output["MaxGamesPerHost"] = each_MaxGamesPerHost;
                Json::Value each_MinFreeGameSlots; ToJsonUtilP(MinFreeGameSlots, each_MinFreeGameSlots); output["MinFreeGameSlots"] = each_MinFreeGameSlots;
                Json::Value each_Status; ToJsonUtilE(Status, each_Status); output["Status"] = each_Status;
                Json::Value each_Timestamp; ToJsonUtilT(Timestamp, each_Timestamp); output["Timestamp"] = each_Timestamp;
                Json::Value each_TitleId; ToJsonUtilS(TitleId, each_TitleId); output["TitleId"] = each_TitleId;
                return output;
            }
        };

        struct AddUserVirtualCurrencyRequest : public PlayFabRequestCommon
        {
            Int32 Amount;
            std::map<std::string, std::string> CustomTags;
            std::string PlayFabId;
            std::string VirtualCurrency;

            AddUserVirtualCurrencyRequest() :
                PlayFabRequestCommon(),
                Amount(),
                CustomTags(),
                PlayFabId(),
                VirtualCurrency()
            {}

            AddUserVirtualCurrencyRequest(const AddUserVirtualCurrencyRequest& src) :
                PlayFabRequestCommon(),
                Amount(src.Amount),
                CustomTags(src.CustomTags),
                PlayFabId(src.PlayFabId),
                VirtualCurrency(src.VirtualCurrency)
            {}

            ~AddUserVirtualCurrencyRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilP(input["Amount"], Amount);
                FromJsonUtilS(input["CustomTags"], CustomTags);
                FromJsonUtilS(input["PlayFabId"], PlayFabId);
                FromJsonUtilS(input["VirtualCurrency"], VirtualCurrency);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_Amount; ToJsonUtilP(Amount, each_Amount); output["Amount"] = each_Amount;
                Json::Value each_CustomTags; ToJsonUtilS(CustomTags, each_CustomTags); output["CustomTags"] = each_CustomTags;
                Json::Value each_PlayFabId; ToJsonUtilS(PlayFabId, each_PlayFabId); output["PlayFabId"] = each_PlayFabId;
                Json::Value each_VirtualCurrency; ToJsonUtilS(VirtualCurrency, each_VirtualCurrency); output["VirtualCurrency"] = each_VirtualCurrency;
                return output;
            }
        };

        struct VirtualCurrencyData : public PlayFabBaseModel
        {
            std::string CurrencyCode;
            std::string DisplayName;
            Boxed<Int32> InitialDeposit;
            Boxed<Int32> RechargeMax;
            Boxed<Int32> RechargeRate;

            VirtualCurrencyData() :
                PlayFabBaseModel(),
                CurrencyCode(),
                DisplayName(),
                InitialDeposit(),
                RechargeMax(),
                RechargeRate()
            {}

            VirtualCurrencyData(const VirtualCurrencyData& src) :
                PlayFabBaseModel(),
                CurrencyCode(src.CurrencyCode),
                DisplayName(src.DisplayName),
                InitialDeposit(src.InitialDeposit),
                RechargeMax(src.RechargeMax),
                RechargeRate(src.RechargeRate)
            {}

            ~VirtualCurrencyData() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["CurrencyCode"], CurrencyCode);
                FromJsonUtilS(input["DisplayName"], DisplayName);
                FromJsonUtilP(input["InitialDeposit"], InitialDeposit);
                FromJsonUtilP(input["RechargeMax"], RechargeMax);
                FromJsonUtilP(input["RechargeRate"], RechargeRate);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_CurrencyCode; ToJsonUtilS(CurrencyCode, each_CurrencyCode); output["CurrencyCode"] = each_CurrencyCode;
                Json::Value each_DisplayName; ToJsonUtilS(DisplayName, each_DisplayName); output["DisplayName"] = each_DisplayName;
                Json::Value each_InitialDeposit; ToJsonUtilP(InitialDeposit, each_InitialDeposit); output["InitialDeposit"] = each_InitialDeposit;
                Json::Value each_RechargeMax; ToJsonUtilP(RechargeMax, each_RechargeMax); output["RechargeMax"] = each_RechargeMax;
                Json::Value each_RechargeRate; ToJsonUtilP(RechargeRate, each_RechargeRate); output["RechargeRate"] = each_RechargeRate;
                return output;
            }
        };

        struct AddVirtualCurrencyTypesRequest : public PlayFabRequestCommon
        {
            std::list<VirtualCurrencyData> VirtualCurrencies;

            AddVirtualCurrencyTypesRequest() :
                PlayFabRequestCommon(),
                VirtualCurrencies()
            {}

            AddVirtualCurrencyTypesRequest(const AddVirtualCurrencyTypesRequest& src) :
                PlayFabRequestCommon(),
                VirtualCurrencies(src.VirtualCurrencies)
            {}

            ~AddVirtualCurrencyTypesRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilO(input["VirtualCurrencies"], VirtualCurrencies);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_VirtualCurrencies; ToJsonUtilO(VirtualCurrencies, each_VirtualCurrencies); output["VirtualCurrencies"] = each_VirtualCurrencies;
                return output;
            }
        };

        struct ApiCondition : public PlayFabBaseModel
        {
            Boxed<Conditionals> HasSignatureOrEncryption;

            ApiCondition() :
                PlayFabBaseModel(),
                HasSignatureOrEncryption()
            {}

            ApiCondition(const ApiCondition& src) :
                PlayFabBaseModel(),
                HasSignatureOrEncryption(src.HasSignatureOrEncryption)
            {}

            ~ApiCondition() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilE(input["HasSignatureOrEncryption"], HasSignatureOrEncryption);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_HasSignatureOrEncryption; ToJsonUtilE(HasSignatureOrEncryption, each_HasSignatureOrEncryption); output["HasSignatureOrEncryption"] = each_HasSignatureOrEncryption;
                return output;
            }
        };

        struct BanInfo : public PlayFabBaseModel
        {
            bool Active;
            std::string BanId;
            Boxed<time_t> Created;
            Boxed<time_t> Expires;
            std::string IPAddress;
            std::string MACAddress;
            std::string PlayFabId;
            std::string Reason;

            BanInfo() :
                PlayFabBaseModel(),
                Active(),
                BanId(),
                Created(),
                Expires(),
                IPAddress(),
                MACAddress(),
                PlayFabId(),
                Reason()
            {}

            BanInfo(const BanInfo& src) :
                PlayFabBaseModel(),
                Active(src.Active),
                BanId(src.BanId),
                Created(src.Created),
                Expires(src.Expires),
                IPAddress(src.IPAddress),
                MACAddress(src.MACAddress),
                PlayFabId(src.PlayFabId),
                Reason(src.Reason)
            {}

            ~BanInfo() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilP(input["Active"], Active);
                FromJsonUtilS(input["BanId"], BanId);
                FromJsonUtilT(input["Created"], Created);
                FromJsonUtilT(input["Expires"], Expires);
                FromJsonUtilS(input["IPAddress"], IPAddress);
                FromJsonUtilS(input["MACAddress"], MACAddress);
                FromJsonUtilS(input["PlayFabId"], PlayFabId);
                FromJsonUtilS(input["Reason"], Reason);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_Active; ToJsonUtilP(Active, each_Active); output["Active"] = each_Active;
                Json::Value each_BanId; ToJsonUtilS(BanId, each_BanId); output["BanId"] = each_BanId;
                Json::Value each_Created; ToJsonUtilT(Created, each_Created); output["Created"] = each_Created;
                Json::Value each_Expires; ToJsonUtilT(Expires, each_Expires); output["Expires"] = each_Expires;
                Json::Value each_IPAddress; ToJsonUtilS(IPAddress, each_IPAddress); output["IPAddress"] = each_IPAddress;
                Json::Value each_MACAddress; ToJsonUtilS(MACAddress, each_MACAddress); output["MACAddress"] = each_MACAddress;
                Json::Value each_PlayFabId; ToJsonUtilS(PlayFabId, each_PlayFabId); output["PlayFabId"] = each_PlayFabId;
                Json::Value each_Reason; ToJsonUtilS(Reason, each_Reason); output["Reason"] = each_Reason;
                return output;
            }
        };

        struct BanRequest : public PlayFabRequestCommon
        {
            Boxed<Uint32> DurationInHours;
            std::string IPAddress;
            std::string MACAddress;
            std::string PlayFabId;
            std::string Reason;

            BanRequest() :
                PlayFabRequestCommon(),
                DurationInHours(),
                IPAddress(),
                MACAddress(),
                PlayFabId(),
                Reason()
            {}

            BanRequest(const BanRequest& src) :
                PlayFabRequestCommon(),
                DurationInHours(src.DurationInHours),
                IPAddress(src.IPAddress),
                MACAddress(src.MACAddress),
                PlayFabId(src.PlayFabId),
                Reason(src.Reason)
            {}

            ~BanRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilP(input["DurationInHours"], DurationInHours);
                FromJsonUtilS(input["IPAddress"], IPAddress);
                FromJsonUtilS(input["MACAddress"], MACAddress);
                FromJsonUtilS(input["PlayFabId"], PlayFabId);
                FromJsonUtilS(input["Reason"], Reason);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_DurationInHours; ToJsonUtilP(DurationInHours, each_DurationInHours); output["DurationInHours"] = each_DurationInHours;
                Json::Value each_IPAddress; ToJsonUtilS(IPAddress, each_IPAddress); output["IPAddress"] = each_IPAddress;
                Json::Value each_MACAddress; ToJsonUtilS(MACAddress, each_MACAddress); output["MACAddress"] = each_MACAddress;
                Json::Value each_PlayFabId; ToJsonUtilS(PlayFabId, each_PlayFabId); output["PlayFabId"] = each_PlayFabId;
                Json::Value each_Reason; ToJsonUtilS(Reason, each_Reason); output["Reason"] = each_Reason;
                return output;
            }
        };

        struct BanUsersRequest : public PlayFabRequestCommon
        {
            std::list<BanRequest> Bans;
            std::map<std::string, std::string> CustomTags;

            BanUsersRequest() :
                PlayFabRequestCommon(),
                Bans(),
                CustomTags()
            {}

            BanUsersRequest(const BanUsersRequest& src) :
                PlayFabRequestCommon(),
                Bans(src.Bans),
                CustomTags(src.CustomTags)
            {}

            ~BanUsersRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilO(input["Bans"], Bans);
                FromJsonUtilS(input["CustomTags"], CustomTags);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_Bans; ToJsonUtilO(Bans, each_Bans); output["Bans"] = each_Bans;
                Json::Value each_CustomTags; ToJsonUtilS(CustomTags, each_CustomTags); output["CustomTags"] = each_CustomTags;
                return output;
            }
        };

        struct BanUsersResult : public PlayFabResultCommon
        {
            std::list<BanInfo> BanData;

            BanUsersResult() :
                PlayFabResultCommon(),
                BanData()
            {}

            BanUsersResult(const BanUsersResult& src) :
                PlayFabResultCommon(),
                BanData(src.BanData)
            {}

            ~BanUsersResult() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilO(input["BanData"], BanData);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_BanData; ToJsonUtilO(BanData, each_BanData); output["BanData"] = each_BanData;
                return output;
            }
        };

        struct BlankResult : public PlayFabResultCommon
        {

            BlankResult() :
                PlayFabResultCommon()
            {}

            BlankResult(const BlankResult&) :
                PlayFabResultCommon()
            {}

            ~BlankResult() = default;

            void FromJson(const Json::Value&) override
            {
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                return output;
            }
        };

        struct CatalogItemBundleInfo : public PlayFabBaseModel
        {
            std::list<std::string> BundledItems;
            std::list<std::string> BundledResultTables;
            std::map<std::string, Uint32> BundledVirtualCurrencies;

            CatalogItemBundleInfo() :
                PlayFabBaseModel(),
                BundledItems(),
                BundledResultTables(),
                BundledVirtualCurrencies()
            {}

            CatalogItemBundleInfo(const CatalogItemBundleInfo& src) :
                PlayFabBaseModel(),
                BundledItems(src.BundledItems),
                BundledResultTables(src.BundledResultTables),
                BundledVirtualCurrencies(src.BundledVirtualCurrencies)
            {}

            ~CatalogItemBundleInfo() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["BundledItems"], BundledItems);
                FromJsonUtilS(input["BundledResultTables"], BundledResultTables);
                FromJsonUtilP(input["BundledVirtualCurrencies"], BundledVirtualCurrencies);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_BundledItems; ToJsonUtilS(BundledItems, each_BundledItems); output["BundledItems"] = each_BundledItems;
                Json::Value each_BundledResultTables; ToJsonUtilS(BundledResultTables, each_BundledResultTables); output["BundledResultTables"] = each_BundledResultTables;
                Json::Value each_BundledVirtualCurrencies; ToJsonUtilP(BundledVirtualCurrencies, each_BundledVirtualCurrencies); output["BundledVirtualCurrencies"] = each_BundledVirtualCurrencies;
                return output;
            }
        };

        struct CatalogItemConsumableInfo : public PlayFabBaseModel
        {
            Boxed<Uint32> UsageCount;
            Boxed<Uint32> UsagePeriod;
            std::string UsagePeriodGroup;

            CatalogItemConsumableInfo() :
                PlayFabBaseModel(),
                UsageCount(),
                UsagePeriod(),
                UsagePeriodGroup()
            {}

            CatalogItemConsumableInfo(const CatalogItemConsumableInfo& src) :
                PlayFabBaseModel(),
                UsageCount(src.UsageCount),
                UsagePeriod(src.UsagePeriod),
                UsagePeriodGroup(src.UsagePeriodGroup)
            {}

            ~CatalogItemConsumableInfo() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilP(input["UsageCount"], UsageCount);
                FromJsonUtilP(input["UsagePeriod"], UsagePeriod);
                FromJsonUtilS(input["UsagePeriodGroup"], UsagePeriodGroup);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_UsageCount; ToJsonUtilP(UsageCount, each_UsageCount); output["UsageCount"] = each_UsageCount;
                Json::Value each_UsagePeriod; ToJsonUtilP(UsagePeriod, each_UsagePeriod); output["UsagePeriod"] = each_UsagePeriod;
                Json::Value each_UsagePeriodGroup; ToJsonUtilS(UsagePeriodGroup, each_UsagePeriodGroup); output["UsagePeriodGroup"] = each_UsagePeriodGroup;
                return output;
            }
        };

        struct CatalogItemContainerInfo : public PlayFabBaseModel
        {
            std::list<std::string> ItemContents;
            std::string KeyItemId;
            std::list<std::string> ResultTableContents;
            std::map<std::string, Uint32> VirtualCurrencyContents;

            CatalogItemContainerInfo() :
                PlayFabBaseModel(),
                ItemContents(),
                KeyItemId(),
                ResultTableContents(),
                VirtualCurrencyContents()
            {}

            CatalogItemContainerInfo(const CatalogItemContainerInfo& src) :
                PlayFabBaseModel(),
                ItemContents(src.ItemContents),
                KeyItemId(src.KeyItemId),
                ResultTableContents(src.ResultTableContents),
                VirtualCurrencyContents(src.VirtualCurrencyContents)
            {}

            ~CatalogItemContainerInfo() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["ItemContents"], ItemContents);
                FromJsonUtilS(input["KeyItemId"], KeyItemId);
                FromJsonUtilS(input["ResultTableContents"], ResultTableContents);
                FromJsonUtilP(input["VirtualCurrencyContents"], VirtualCurrencyContents);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_ItemContents; ToJsonUtilS(ItemContents, each_ItemContents); output["ItemContents"] = each_ItemContents;
                Json::Value each_KeyItemId; ToJsonUtilS(KeyItemId, each_KeyItemId); output["KeyItemId"] = each_KeyItemId;
                Json::Value each_ResultTableContents; ToJsonUtilS(ResultTableContents, each_ResultTableContents); output["ResultTableContents"] = each_ResultTableContents;
                Json::Value each_VirtualCurrencyContents; ToJsonUtilP(VirtualCurrencyContents, each_VirtualCurrencyContents); output["VirtualCurrencyContents"] = each_VirtualCurrencyContents;
                return output;
            }
        };

        struct CatalogItem : public PlayFabBaseModel
        {
            Boxed<CatalogItemBundleInfo> Bundle;
            bool CanBecomeCharacter;
            std::string CatalogVersion;
            Boxed<CatalogItemConsumableInfo> Consumable;
            Boxed<CatalogItemContainerInfo> Container;
            std::string CustomData;
            std::string Description;
            std::string DisplayName;
            Int32 InitialLimitedEditionCount;
            bool IsLimitedEdition;
            bool IsStackable;
            bool IsTradable;
            std::string ItemClass;
            std::string ItemId;
            std::string ItemImageUrl;
            std::map<std::string, Uint32> RealCurrencyPrices;
            std::list<std::string> Tags;
            std::map<std::string, Uint32> VirtualCurrencyPrices;

            CatalogItem() :
                PlayFabBaseModel(),
                Bundle(),
                CanBecomeCharacter(),
                CatalogVersion(),
                Consumable(),
                Container(),
                CustomData(),
                Description(),
                DisplayName(),
                InitialLimitedEditionCount(),
                IsLimitedEdition(),
                IsStackable(),
                IsTradable(),
                ItemClass(),
                ItemId(),
                ItemImageUrl(),
                RealCurrencyPrices(),
                Tags(),
                VirtualCurrencyPrices()
            {}

            CatalogItem(const CatalogItem& src) :
                PlayFabBaseModel(),
                Bundle(src.Bundle),
                CanBecomeCharacter(src.CanBecomeCharacter),
                CatalogVersion(src.CatalogVersion),
                Consumable(src.Consumable),
                Container(src.Container),
                CustomData(src.CustomData),
                Description(src.Description),
                DisplayName(src.DisplayName),
                InitialLimitedEditionCount(src.InitialLimitedEditionCount),
                IsLimitedEdition(src.IsLimitedEdition),
                IsStackable(src.IsStackable),
                IsTradable(src.IsTradable),
                ItemClass(src.ItemClass),
                ItemId(src.ItemId),
                ItemImageUrl(src.ItemImageUrl),
                RealCurrencyPrices(src.RealCurrencyPrices),
                Tags(src.Tags),
                VirtualCurrencyPrices(src.VirtualCurrencyPrices)
            {}

            ~CatalogItem() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilO(input["Bundle"], Bundle);
                FromJsonUtilP(input["CanBecomeCharacter"], CanBecomeCharacter);
                FromJsonUtilS(input["CatalogVersion"], CatalogVersion);
                FromJsonUtilO(input["Consumable"], Consumable);
                FromJsonUtilO(input["Container"], Container);
                FromJsonUtilS(input["CustomData"], CustomData);
                FromJsonUtilS(input["Description"], Description);
                FromJsonUtilS(input["DisplayName"], DisplayName);
                FromJsonUtilP(input["InitialLimitedEditionCount"], InitialLimitedEditionCount);
                FromJsonUtilP(input["IsLimitedEdition"], IsLimitedEdition);
                FromJsonUtilP(input["IsStackable"], IsStackable);
                FromJsonUtilP(input["IsTradable"], IsTradable);
                FromJsonUtilS(input["ItemClass"], ItemClass);
                FromJsonUtilS(input["ItemId"], ItemId);
                FromJsonUtilS(input["ItemImageUrl"], ItemImageUrl);
                FromJsonUtilP(input["RealCurrencyPrices"], RealCurrencyPrices);
                FromJsonUtilS(input["Tags"], Tags);
                FromJsonUtilP(input["VirtualCurrencyPrices"], VirtualCurrencyPrices);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_Bundle; ToJsonUtilO(Bundle, each_Bundle); output["Bundle"] = each_Bundle;
                Json::Value each_CanBecomeCharacter; ToJsonUtilP(CanBecomeCharacter, each_CanBecomeCharacter); output["CanBecomeCharacter"] = each_CanBecomeCharacter;
                Json::Value each_CatalogVersion; ToJsonUtilS(CatalogVersion, each_CatalogVersion); output["CatalogVersion"] = each_CatalogVersion;
                Json::Value each_Consumable; ToJsonUtilO(Consumable, each_Consumable); output["Consumable"] = each_Consumable;
                Json::Value each_Container; ToJsonUtilO(Container, each_Container); output["Container"] = each_Container;
                Json::Value each_CustomData; ToJsonUtilS(CustomData, each_CustomData); output["CustomData"] = each_CustomData;
                Json::Value each_Description; ToJsonUtilS(Description, each_Description); output["Description"] = each_Description;
                Json::Value each_DisplayName; ToJsonUtilS(DisplayName, each_DisplayName); output["DisplayName"] = each_DisplayName;
                Json::Value each_InitialLimitedEditionCount; ToJsonUtilP(InitialLimitedEditionCount, each_InitialLimitedEditionCount); output["InitialLimitedEditionCount"] = each_InitialLimitedEditionCount;
                Json::Value each_IsLimitedEdition; ToJsonUtilP(IsLimitedEdition, each_IsLimitedEdition); output["IsLimitedEdition"] = each_IsLimitedEdition;
                Json::Value each_IsStackable; ToJsonUtilP(IsStackable, each_IsStackable); output["IsStackable"] = each_IsStackable;
                Json::Value each_IsTradable; ToJsonUtilP(IsTradable, each_IsTradable); output["IsTradable"] = each_IsTradable;
                Json::Value each_ItemClass; ToJsonUtilS(ItemClass, each_ItemClass); output["ItemClass"] = each_ItemClass;
                Json::Value each_ItemId; ToJsonUtilS(ItemId, each_ItemId); output["ItemId"] = each_ItemId;
                Json::Value each_ItemImageUrl; ToJsonUtilS(ItemImageUrl, each_ItemImageUrl); output["ItemImageUrl"] = each_ItemImageUrl;
                Json::Value each_RealCurrencyPrices; ToJsonUtilP(RealCurrencyPrices, each_RealCurrencyPrices); output["RealCurrencyPrices"] = each_RealCurrencyPrices;
                Json::Value each_Tags; ToJsonUtilS(Tags, each_Tags); output["Tags"] = each_Tags;
                Json::Value each_VirtualCurrencyPrices; ToJsonUtilP(VirtualCurrencyPrices, each_VirtualCurrencyPrices); output["VirtualCurrencyPrices"] = each_VirtualCurrencyPrices;
                return output;
            }
        };

        struct CheckLimitedEditionItemAvailabilityRequest : public PlayFabRequestCommon
        {
            std::string CatalogVersion;
            std::string ItemId;

            CheckLimitedEditionItemAvailabilityRequest() :
                PlayFabRequestCommon(),
                CatalogVersion(),
                ItemId()
            {}

            CheckLimitedEditionItemAvailabilityRequest(const CheckLimitedEditionItemAvailabilityRequest& src) :
                PlayFabRequestCommon(),
                CatalogVersion(src.CatalogVersion),
                ItemId(src.ItemId)
            {}

            ~CheckLimitedEditionItemAvailabilityRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["CatalogVersion"], CatalogVersion);
                FromJsonUtilS(input["ItemId"], ItemId);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_CatalogVersion; ToJsonUtilS(CatalogVersion, each_CatalogVersion); output["CatalogVersion"] = each_CatalogVersion;
                Json::Value each_ItemId; ToJsonUtilS(ItemId, each_ItemId); output["ItemId"] = each_ItemId;
                return output;
            }
        };

        struct CheckLimitedEditionItemAvailabilityResult : public PlayFabResultCommon
        {
            Int32 Amount;

            CheckLimitedEditionItemAvailabilityResult() :
                PlayFabResultCommon(),
                Amount()
            {}

            CheckLimitedEditionItemAvailabilityResult(const CheckLimitedEditionItemAvailabilityResult& src) :
                PlayFabResultCommon(),
                Amount(src.Amount)
            {}

            ~CheckLimitedEditionItemAvailabilityResult() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilP(input["Amount"], Amount);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_Amount; ToJsonUtilP(Amount, each_Amount); output["Amount"] = each_Amount;
                return output;
            }
        };

        struct CloudScriptFile : public PlayFabBaseModel
        {
            std::string FileContents;
            std::string Filename;

            CloudScriptFile() :
                PlayFabBaseModel(),
                FileContents(),
                Filename()
            {}

            CloudScriptFile(const CloudScriptFile& src) :
                PlayFabBaseModel(),
                FileContents(src.FileContents),
                Filename(src.Filename)
            {}

            ~CloudScriptFile() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["FileContents"], FileContents);
                FromJsonUtilS(input["Filename"], Filename);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_FileContents; ToJsonUtilS(FileContents, each_FileContents); output["FileContents"] = each_FileContents;
                Json::Value each_Filename; ToJsonUtilS(Filename, each_Filename); output["Filename"] = each_Filename;
                return output;
            }
        };

        struct CloudScriptTaskParameter : public PlayFabBaseModel
        {
            Json::Value Argument;
            std::string FunctionName;

            CloudScriptTaskParameter() :
                PlayFabBaseModel(),
                Argument(),
                FunctionName()
            {}

            CloudScriptTaskParameter(const CloudScriptTaskParameter& src) :
                PlayFabBaseModel(),
                Argument(src.Argument),
                FunctionName(src.FunctionName)
            {}

            ~CloudScriptTaskParameter() = default;

            void FromJson(const Json::Value& input) override
            {
                Argument = input["Argument"];
                FromJsonUtilS(input["FunctionName"], FunctionName);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                output["Argument"] = Argument;
                Json::Value each_FunctionName; ToJsonUtilS(FunctionName, each_FunctionName); output["FunctionName"] = each_FunctionName;
                return output;
            }
        };

        struct ScriptExecutionError : public PlayFabBaseModel
        {
            std::string Error;
            std::string Message;
            std::string StackTrace;

            ScriptExecutionError() :
                PlayFabBaseModel(),
                Error(),
                Message(),
                StackTrace()
            {}

            ScriptExecutionError(const ScriptExecutionError& src) :
                PlayFabBaseModel(),
                Error(src.Error),
                Message(src.Message),
                StackTrace(src.StackTrace)
            {}

            ~ScriptExecutionError() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["Error"], Error);
                FromJsonUtilS(input["Message"], Message);
                FromJsonUtilS(input["StackTrace"], StackTrace);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_Error; ToJsonUtilS(Error, each_Error); output["Error"] = each_Error;
                Json::Value each_Message; ToJsonUtilS(Message, each_Message); output["Message"] = each_Message;
                Json::Value each_StackTrace; ToJsonUtilS(StackTrace, each_StackTrace); output["StackTrace"] = each_StackTrace;
                return output;
            }
        };

        struct LogStatement : public PlayFabBaseModel
        {
            Json::Value Data;
            std::string Level;
            std::string Message;

            LogStatement() :
                PlayFabBaseModel(),
                Data(),
                Level(),
                Message()
            {}

            LogStatement(const LogStatement& src) :
                PlayFabBaseModel(),
                Data(src.Data),
                Level(src.Level),
                Message(src.Message)
            {}

            ~LogStatement() = default;

            void FromJson(const Json::Value& input) override
            {
                Data = input["Data"];
                FromJsonUtilS(input["Level"], Level);
                FromJsonUtilS(input["Message"], Message);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                output["Data"] = Data;
                Json::Value each_Level; ToJsonUtilS(Level, each_Level); output["Level"] = each_Level;
                Json::Value each_Message; ToJsonUtilS(Message, each_Message); output["Message"] = each_Message;
                return output;
            }
        };

        struct ExecuteCloudScriptResult : public PlayFabResultCommon
        {
            Int32 APIRequestsIssued;
            Boxed<ScriptExecutionError> Error;
            double ExecutionTimeSeconds;
            std::string FunctionName;
            Json::Value FunctionResult;
            Boxed<bool> FunctionResultTooLarge;
            Int32 HttpRequestsIssued;
            std::list<LogStatement> Logs;
            Boxed<bool> LogsTooLarge;
            Uint32 MemoryConsumedBytes;
            double ProcessorTimeSeconds;
            Int32 Revision;

            ExecuteCloudScriptResult() :
                PlayFabResultCommon(),
                APIRequestsIssued(),
                Error(),
                ExecutionTimeSeconds(),
                FunctionName(),
                FunctionResult(),
                FunctionResultTooLarge(),
                HttpRequestsIssued(),
                Logs(),
                LogsTooLarge(),
                MemoryConsumedBytes(),
                ProcessorTimeSeconds(),
                Revision()
            {}

            ExecuteCloudScriptResult(const ExecuteCloudScriptResult& src) :
                PlayFabResultCommon(),
                APIRequestsIssued(src.APIRequestsIssued),
                Error(src.Error),
                ExecutionTimeSeconds(src.ExecutionTimeSeconds),
                FunctionName(src.FunctionName),
                FunctionResult(src.FunctionResult),
                FunctionResultTooLarge(src.FunctionResultTooLarge),
                HttpRequestsIssued(src.HttpRequestsIssued),
                Logs(src.Logs),
                LogsTooLarge(src.LogsTooLarge),
                MemoryConsumedBytes(src.MemoryConsumedBytes),
                ProcessorTimeSeconds(src.ProcessorTimeSeconds),
                Revision(src.Revision)
            {}

            ~ExecuteCloudScriptResult() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilP(input["APIRequestsIssued"], APIRequestsIssued);
                FromJsonUtilO(input["Error"], Error);
                FromJsonUtilP(input["ExecutionTimeSeconds"], ExecutionTimeSeconds);
                FromJsonUtilS(input["FunctionName"], FunctionName);
                FunctionResult = input["FunctionResult"];
                FromJsonUtilP(input["FunctionResultTooLarge"], FunctionResultTooLarge);
                FromJsonUtilP(input["HttpRequestsIssued"], HttpRequestsIssued);
                FromJsonUtilO(input["Logs"], Logs);
                FromJsonUtilP(input["LogsTooLarge"], LogsTooLarge);
                FromJsonUtilP(input["MemoryConsumedBytes"], MemoryConsumedBytes);
                FromJsonUtilP(input["ProcessorTimeSeconds"], ProcessorTimeSeconds);
                FromJsonUtilP(input["Revision"], Revision);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_APIRequestsIssued; ToJsonUtilP(APIRequestsIssued, each_APIRequestsIssued); output["APIRequestsIssued"] = each_APIRequestsIssued;
                Json::Value each_Error; ToJsonUtilO(Error, each_Error); output["Error"] = each_Error;
                Json::Value each_ExecutionTimeSeconds; ToJsonUtilP(ExecutionTimeSeconds, each_ExecutionTimeSeconds); output["ExecutionTimeSeconds"] = each_ExecutionTimeSeconds;
                Json::Value each_FunctionName; ToJsonUtilS(FunctionName, each_FunctionName); output["FunctionName"] = each_FunctionName;
                output["FunctionResult"] = FunctionResult;
                Json::Value each_FunctionResultTooLarge; ToJsonUtilP(FunctionResultTooLarge, each_FunctionResultTooLarge); output["FunctionResultTooLarge"] = each_FunctionResultTooLarge;
                Json::Value each_HttpRequestsIssued; ToJsonUtilP(HttpRequestsIssued, each_HttpRequestsIssued); output["HttpRequestsIssued"] = each_HttpRequestsIssued;
                Json::Value each_Logs; ToJsonUtilO(Logs, each_Logs); output["Logs"] = each_Logs;
                Json::Value each_LogsTooLarge; ToJsonUtilP(LogsTooLarge, each_LogsTooLarge); output["LogsTooLarge"] = each_LogsTooLarge;
                Json::Value each_MemoryConsumedBytes; ToJsonUtilP(MemoryConsumedBytes, each_MemoryConsumedBytes); output["MemoryConsumedBytes"] = each_MemoryConsumedBytes;
                Json::Value each_ProcessorTimeSeconds; ToJsonUtilP(ProcessorTimeSeconds, each_ProcessorTimeSeconds); output["ProcessorTimeSeconds"] = each_ProcessorTimeSeconds;
                Json::Value each_Revision; ToJsonUtilP(Revision, each_Revision); output["Revision"] = each_Revision;
                return output;
            }
        };

        struct CloudScriptTaskSummary : public PlayFabBaseModel
        {
            Boxed<time_t> CompletedAt;
            Boxed<double> EstimatedSecondsRemaining;
            Boxed<double> PercentComplete;
            Boxed<ExecuteCloudScriptResult> Result;
            std::string ScheduledByUserId;
            time_t StartedAt;
            Boxed<TaskInstanceStatus> Status;
            Boxed<NameIdentifier> TaskIdentifier;
            std::string TaskInstanceId;

            CloudScriptTaskSummary() :
                PlayFabBaseModel(),
                CompletedAt(),
                EstimatedSecondsRemaining(),
                PercentComplete(),
                Result(),
                ScheduledByUserId(),
                StartedAt(),
                Status(),
                TaskIdentifier(),
                TaskInstanceId()
            {}

            CloudScriptTaskSummary(const CloudScriptTaskSummary& src) :
                PlayFabBaseModel(),
                CompletedAt(src.CompletedAt),
                EstimatedSecondsRemaining(src.EstimatedSecondsRemaining),
                PercentComplete(src.PercentComplete),
                Result(src.Result),
                ScheduledByUserId(src.ScheduledByUserId),
                StartedAt(src.StartedAt),
                Status(src.Status),
                TaskIdentifier(src.TaskIdentifier),
                TaskInstanceId(src.TaskInstanceId)
            {}

            ~CloudScriptTaskSummary() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilT(input["CompletedAt"], CompletedAt);
                FromJsonUtilP(input["EstimatedSecondsRemaining"], EstimatedSecondsRemaining);
                FromJsonUtilP(input["PercentComplete"], PercentComplete);
                FromJsonUtilO(input["Result"], Result);
                FromJsonUtilS(input["ScheduledByUserId"], ScheduledByUserId);
                FromJsonUtilT(input["StartedAt"], StartedAt);
                FromJsonUtilE(input["Status"], Status);
                FromJsonUtilO(input["TaskIdentifier"], TaskIdentifier);
                FromJsonUtilS(input["TaskInstanceId"], TaskInstanceId);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_CompletedAt; ToJsonUtilT(CompletedAt, each_CompletedAt); output["CompletedAt"] = each_CompletedAt;
                Json::Value each_EstimatedSecondsRemaining; ToJsonUtilP(EstimatedSecondsRemaining, each_EstimatedSecondsRemaining); output["EstimatedSecondsRemaining"] = each_EstimatedSecondsRemaining;
                Json::Value each_PercentComplete; ToJsonUtilP(PercentComplete, each_PercentComplete); output["PercentComplete"] = each_PercentComplete;
                Json::Value each_Result; ToJsonUtilO(Result, each_Result); output["Result"] = each_Result;
                Json::Value each_ScheduledByUserId; ToJsonUtilS(ScheduledByUserId, each_ScheduledByUserId); output["ScheduledByUserId"] = each_ScheduledByUserId;
                Json::Value each_StartedAt; ToJsonUtilT(StartedAt, each_StartedAt); output["StartedAt"] = each_StartedAt;
                Json::Value each_Status; ToJsonUtilE(Status, each_Status); output["Status"] = each_Status;
                Json::Value each_TaskIdentifier; ToJsonUtilO(TaskIdentifier, each_TaskIdentifier); output["TaskIdentifier"] = each_TaskIdentifier;
                Json::Value each_TaskInstanceId; ToJsonUtilS(TaskInstanceId, each_TaskInstanceId); output["TaskInstanceId"] = each_TaskInstanceId;
                return output;
            }
        };

        struct CloudScriptVersionStatus : public PlayFabBaseModel
        {
            Int32 LatestRevision;
            Int32 PublishedRevision;
            Int32 Version;

            CloudScriptVersionStatus() :
                PlayFabBaseModel(),
                LatestRevision(),
                PublishedRevision(),
                Version()
            {}

            CloudScriptVersionStatus(const CloudScriptVersionStatus& src) :
                PlayFabBaseModel(),
                LatestRevision(src.LatestRevision),
                PublishedRevision(src.PublishedRevision),
                Version(src.Version)
            {}

            ~CloudScriptVersionStatus() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilP(input["LatestRevision"], LatestRevision);
                FromJsonUtilP(input["PublishedRevision"], PublishedRevision);
                FromJsonUtilP(input["Version"], Version);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_LatestRevision; ToJsonUtilP(LatestRevision, each_LatestRevision); output["LatestRevision"] = each_LatestRevision;
                Json::Value each_PublishedRevision; ToJsonUtilP(PublishedRevision, each_PublishedRevision); output["PublishedRevision"] = each_PublishedRevision;
                Json::Value each_Version; ToJsonUtilP(Version, each_Version); output["Version"] = each_Version;
                return output;
            }
        };

        struct ContactEmailInfo : public PlayFabBaseModel
        {
            std::string EmailAddress;
            std::string Name;
            Boxed<EmailVerificationStatus> VerificationStatus;

            ContactEmailInfo() :
                PlayFabBaseModel(),
                EmailAddress(),
                Name(),
                VerificationStatus()
            {}

            ContactEmailInfo(const ContactEmailInfo& src) :
                PlayFabBaseModel(),
                EmailAddress(src.EmailAddress),
                Name(src.Name),
                VerificationStatus(src.VerificationStatus)
            {}

            ~ContactEmailInfo() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["EmailAddress"], EmailAddress);
                FromJsonUtilS(input["Name"], Name);
                FromJsonUtilE(input["VerificationStatus"], VerificationStatus);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_EmailAddress; ToJsonUtilS(EmailAddress, each_EmailAddress); output["EmailAddress"] = each_EmailAddress;
                Json::Value each_Name; ToJsonUtilS(Name, each_Name); output["Name"] = each_Name;
                Json::Value each_VerificationStatus; ToJsonUtilE(VerificationStatus, each_VerificationStatus); output["VerificationStatus"] = each_VerificationStatus;
                return output;
            }
        };

        struct ContactEmailInfoModel : public PlayFabBaseModel
        {
            std::string EmailAddress;
            std::string Name;
            Boxed<EmailVerificationStatus> VerificationStatus;

            ContactEmailInfoModel() :
                PlayFabBaseModel(),
                EmailAddress(),
                Name(),
                VerificationStatus()
            {}

            ContactEmailInfoModel(const ContactEmailInfoModel& src) :
                PlayFabBaseModel(),
                EmailAddress(src.EmailAddress),
                Name(src.Name),
                VerificationStatus(src.VerificationStatus)
            {}

            ~ContactEmailInfoModel() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["EmailAddress"], EmailAddress);
                FromJsonUtilS(input["Name"], Name);
                FromJsonUtilE(input["VerificationStatus"], VerificationStatus);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_EmailAddress; ToJsonUtilS(EmailAddress, each_EmailAddress); output["EmailAddress"] = each_EmailAddress;
                Json::Value each_Name; ToJsonUtilS(Name, each_Name); output["Name"] = each_Name;
                Json::Value each_VerificationStatus; ToJsonUtilE(VerificationStatus, each_VerificationStatus); output["VerificationStatus"] = each_VerificationStatus;
                return output;
            }
        };

        struct ContentInfo : public PlayFabBaseModel
        {
            std::string Key;
            time_t LastModified;
            Uint32 Size;

            ContentInfo() :
                PlayFabBaseModel(),
                Key(),
                LastModified(),
                Size()
            {}

            ContentInfo(const ContentInfo& src) :
                PlayFabBaseModel(),
                Key(src.Key),
                LastModified(src.LastModified),
                Size(src.Size)
            {}

            ~ContentInfo() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["Key"], Key);
                FromJsonUtilT(input["LastModified"], LastModified);
                FromJsonUtilP(input["Size"], Size);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_Key; ToJsonUtilS(Key, each_Key); output["Key"] = each_Key;
                Json::Value each_LastModified; ToJsonUtilT(LastModified, each_LastModified); output["LastModified"] = each_LastModified;
                Json::Value each_Size; ToJsonUtilP(Size, each_Size); output["Size"] = each_Size;
                return output;
            }
        };

        struct CreateActionsOnPlayerSegmentTaskRequest : public PlayFabRequestCommon
        {
            std::map<std::string, std::string> CustomTags;
            std::string Description;
            bool IsActive;
            std::string Name;
            ActionsOnPlayersInSegmentTaskParameter Parameter;
            std::string Schedule;

            CreateActionsOnPlayerSegmentTaskRequest() :
                PlayFabRequestCommon(),
                CustomTags(),
                Description(),
                IsActive(),
                Name(),
                Parameter(),
                Schedule()
            {}

            CreateActionsOnPlayerSegmentTaskRequest(const CreateActionsOnPlayerSegmentTaskRequest& src) :
                PlayFabRequestCommon(),
                CustomTags(src.CustomTags),
                Description(src.Description),
                IsActive(src.IsActive),
                Name(src.Name),
                Parameter(src.Parameter),
                Schedule(src.Schedule)
            {}

            ~CreateActionsOnPlayerSegmentTaskRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["CustomTags"], CustomTags);
                FromJsonUtilS(input["Description"], Description);
                FromJsonUtilP(input["IsActive"], IsActive);
                FromJsonUtilS(input["Name"], Name);
                FromJsonUtilO(input["Parameter"], Parameter);
                FromJsonUtilS(input["Schedule"], Schedule);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_CustomTags; ToJsonUtilS(CustomTags, each_CustomTags); output["CustomTags"] = each_CustomTags;
                Json::Value each_Description; ToJsonUtilS(Description, each_Description); output["Description"] = each_Description;
                Json::Value each_IsActive; ToJsonUtilP(IsActive, each_IsActive); output["IsActive"] = each_IsActive;
                Json::Value each_Name; ToJsonUtilS(Name, each_Name); output["Name"] = each_Name;
                Json::Value each_Parameter; ToJsonUtilO(Parameter, each_Parameter); output["Parameter"] = each_Parameter;
                Json::Value each_Schedule; ToJsonUtilS(Schedule, each_Schedule); output["Schedule"] = each_Schedule;
                return output;
            }
        };

        struct CreateCloudScriptTaskRequest : public PlayFabRequestCommon
        {
            std::map<std::string, std::string> CustomTags;
            std::string Description;
            bool IsActive;
            std::string Name;
            CloudScriptTaskParameter Parameter;
            std::string Schedule;

            CreateCloudScriptTaskRequest() :
                PlayFabRequestCommon(),
                CustomTags(),
                Description(),
                IsActive(),
                Name(),
                Parameter(),
                Schedule()
            {}

            CreateCloudScriptTaskRequest(const CreateCloudScriptTaskRequest& src) :
                PlayFabRequestCommon(),
                CustomTags(src.CustomTags),
                Description(src.Description),
                IsActive(src.IsActive),
                Name(src.Name),
                Parameter(src.Parameter),
                Schedule(src.Schedule)
            {}

            ~CreateCloudScriptTaskRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["CustomTags"], CustomTags);
                FromJsonUtilS(input["Description"], Description);
                FromJsonUtilP(input["IsActive"], IsActive);
                FromJsonUtilS(input["Name"], Name);
                FromJsonUtilO(input["Parameter"], Parameter);
                FromJsonUtilS(input["Schedule"], Schedule);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_CustomTags; ToJsonUtilS(CustomTags, each_CustomTags); output["CustomTags"] = each_CustomTags;
                Json::Value each_Description; ToJsonUtilS(Description, each_Description); output["Description"] = each_Description;
                Json::Value each_IsActive; ToJsonUtilP(IsActive, each_IsActive); output["IsActive"] = each_IsActive;
                Json::Value each_Name; ToJsonUtilS(Name, each_Name); output["Name"] = each_Name;
                Json::Value each_Parameter; ToJsonUtilO(Parameter, each_Parameter); output["Parameter"] = each_Parameter;
                Json::Value each_Schedule; ToJsonUtilS(Schedule, each_Schedule); output["Schedule"] = each_Schedule;
                return output;
            }
        };

        struct InsightsScalingTaskParameter : public PlayFabBaseModel
        {
            Int32 Level;

            InsightsScalingTaskParameter() :
                PlayFabBaseModel(),
                Level()
            {}

            InsightsScalingTaskParameter(const InsightsScalingTaskParameter& src) :
                PlayFabBaseModel(),
                Level(src.Level)
            {}

            ~InsightsScalingTaskParameter() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilP(input["Level"], Level);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_Level; ToJsonUtilP(Level, each_Level); output["Level"] = each_Level;
                return output;
            }
        };

        struct CreateInsightsScheduledScalingTaskRequest : public PlayFabRequestCommon
        {
            std::map<std::string, std::string> CustomTags;
            std::string Description;
            bool IsActive;
            std::string Name;
            InsightsScalingTaskParameter Parameter;
            std::string Schedule;

            CreateInsightsScheduledScalingTaskRequest() :
                PlayFabRequestCommon(),
                CustomTags(),
                Description(),
                IsActive(),
                Name(),
                Parameter(),
                Schedule()
            {}

            CreateInsightsScheduledScalingTaskRequest(const CreateInsightsScheduledScalingTaskRequest& src) :
                PlayFabRequestCommon(),
                CustomTags(src.CustomTags),
                Description(src.Description),
                IsActive(src.IsActive),
                Name(src.Name),
                Parameter(src.Parameter),
                Schedule(src.Schedule)
            {}

            ~CreateInsightsScheduledScalingTaskRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["CustomTags"], CustomTags);
                FromJsonUtilS(input["Description"], Description);
                FromJsonUtilP(input["IsActive"], IsActive);
                FromJsonUtilS(input["Name"], Name);
                FromJsonUtilO(input["Parameter"], Parameter);
                FromJsonUtilS(input["Schedule"], Schedule);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_CustomTags; ToJsonUtilS(CustomTags, each_CustomTags); output["CustomTags"] = each_CustomTags;
                Json::Value each_Description; ToJsonUtilS(Description, each_Description); output["Description"] = each_Description;
                Json::Value each_IsActive; ToJsonUtilP(IsActive, each_IsActive); output["IsActive"] = each_IsActive;
                Json::Value each_Name; ToJsonUtilS(Name, each_Name); output["Name"] = each_Name;
                Json::Value each_Parameter; ToJsonUtilO(Parameter, each_Parameter); output["Parameter"] = each_Parameter;
                Json::Value each_Schedule; ToJsonUtilS(Schedule, each_Schedule); output["Schedule"] = each_Schedule;
                return output;
            }
        };

        struct OpenIdIssuerInformation : public PlayFabBaseModel
        {
            std::string AuthorizationUrl;
            std::string Issuer;
            Json::Value JsonWebKeySet;
            std::string TokenUrl;

            OpenIdIssuerInformation() :
                PlayFabBaseModel(),
                AuthorizationUrl(),
                Issuer(),
                JsonWebKeySet(),
                TokenUrl()
            {}

            OpenIdIssuerInformation(const OpenIdIssuerInformation& src) :
                PlayFabBaseModel(),
                AuthorizationUrl(src.AuthorizationUrl),
                Issuer(src.Issuer),
                JsonWebKeySet(src.JsonWebKeySet),
                TokenUrl(src.TokenUrl)
            {}

            ~OpenIdIssuerInformation() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["AuthorizationUrl"], AuthorizationUrl);
                FromJsonUtilS(input["Issuer"], Issuer);
                JsonWebKeySet = input["JsonWebKeySet"];
                FromJsonUtilS(input["TokenUrl"], TokenUrl);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_AuthorizationUrl; ToJsonUtilS(AuthorizationUrl, each_AuthorizationUrl); output["AuthorizationUrl"] = each_AuthorizationUrl;
                Json::Value each_Issuer; ToJsonUtilS(Issuer, each_Issuer); output["Issuer"] = each_Issuer;
                output["JsonWebKeySet"] = JsonWebKeySet;
                Json::Value each_TokenUrl; ToJsonUtilS(TokenUrl, each_TokenUrl); output["TokenUrl"] = each_TokenUrl;
                return output;
            }
        };

        struct CreateOpenIdConnectionRequest : public PlayFabRequestCommon
        {
            std::string ClientId;
            std::string ClientSecret;
            std::string ConnectionId;
            std::string IssuerDiscoveryUrl;
            Boxed<OpenIdIssuerInformation> IssuerInformation;

            CreateOpenIdConnectionRequest() :
                PlayFabRequestCommon(),
                ClientId(),
                ClientSecret(),
                ConnectionId(),
                IssuerDiscoveryUrl(),
                IssuerInformation()
            {}

            CreateOpenIdConnectionRequest(const CreateOpenIdConnectionRequest& src) :
                PlayFabRequestCommon(),
                ClientId(src.ClientId),
                ClientSecret(src.ClientSecret),
                ConnectionId(src.ConnectionId),
                IssuerDiscoveryUrl(src.IssuerDiscoveryUrl),
                IssuerInformation(src.IssuerInformation)
            {}

            ~CreateOpenIdConnectionRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["ClientId"], ClientId);
                FromJsonUtilS(input["ClientSecret"], ClientSecret);
                FromJsonUtilS(input["ConnectionId"], ConnectionId);
                FromJsonUtilS(input["IssuerDiscoveryUrl"], IssuerDiscoveryUrl);
                FromJsonUtilO(input["IssuerInformation"], IssuerInformation);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_ClientId; ToJsonUtilS(ClientId, each_ClientId); output["ClientId"] = each_ClientId;
                Json::Value each_ClientSecret; ToJsonUtilS(ClientSecret, each_ClientSecret); output["ClientSecret"] = each_ClientSecret;
                Json::Value each_ConnectionId; ToJsonUtilS(ConnectionId, each_ConnectionId); output["ConnectionId"] = each_ConnectionId;
                Json::Value each_IssuerDiscoveryUrl; ToJsonUtilS(IssuerDiscoveryUrl, each_IssuerDiscoveryUrl); output["IssuerDiscoveryUrl"] = each_IssuerDiscoveryUrl;
                Json::Value each_IssuerInformation; ToJsonUtilO(IssuerInformation, each_IssuerInformation); output["IssuerInformation"] = each_IssuerInformation;
                return output;
            }
        };

        struct CreatePlayerSharedSecretRequest : public PlayFabRequestCommon
        {
            std::string FriendlyName;

            CreatePlayerSharedSecretRequest() :
                PlayFabRequestCommon(),
                FriendlyName()
            {}

            CreatePlayerSharedSecretRequest(const CreatePlayerSharedSecretRequest& src) :
                PlayFabRequestCommon(),
                FriendlyName(src.FriendlyName)
            {}

            ~CreatePlayerSharedSecretRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["FriendlyName"], FriendlyName);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_FriendlyName; ToJsonUtilS(FriendlyName, each_FriendlyName); output["FriendlyName"] = each_FriendlyName;
                return output;
            }
        };

        struct CreatePlayerSharedSecretResult : public PlayFabResultCommon
        {
            std::string SecretKey;

            CreatePlayerSharedSecretResult() :
                PlayFabResultCommon(),
                SecretKey()
            {}

            CreatePlayerSharedSecretResult(const CreatePlayerSharedSecretResult& src) :
                PlayFabResultCommon(),
                SecretKey(src.SecretKey)
            {}

            ~CreatePlayerSharedSecretResult() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["SecretKey"], SecretKey);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_SecretKey; ToJsonUtilS(SecretKey, each_SecretKey); output["SecretKey"] = each_SecretKey;
                return output;
            }
        };

        struct CreatePlayerStatisticDefinitionRequest : public PlayFabRequestCommon
        {
            Boxed<StatisticAggregationMethod> AggregationMethod;
            std::map<std::string, std::string> CustomTags;
            std::string StatisticName;
            Boxed<StatisticResetIntervalOption> VersionChangeInterval;

            CreatePlayerStatisticDefinitionRequest() :
                PlayFabRequestCommon(),
                AggregationMethod(),
                CustomTags(),
                StatisticName(),
                VersionChangeInterval()
            {}

            CreatePlayerStatisticDefinitionRequest(const CreatePlayerStatisticDefinitionRequest& src) :
                PlayFabRequestCommon(),
                AggregationMethod(src.AggregationMethod),
                CustomTags(src.CustomTags),
                StatisticName(src.StatisticName),
                VersionChangeInterval(src.VersionChangeInterval)
            {}

            ~CreatePlayerStatisticDefinitionRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilE(input["AggregationMethod"], AggregationMethod);
                FromJsonUtilS(input["CustomTags"], CustomTags);
                FromJsonUtilS(input["StatisticName"], StatisticName);
                FromJsonUtilE(input["VersionChangeInterval"], VersionChangeInterval);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_AggregationMethod; ToJsonUtilE(AggregationMethod, each_AggregationMethod); output["AggregationMethod"] = each_AggregationMethod;
                Json::Value each_CustomTags; ToJsonUtilS(CustomTags, each_CustomTags); output["CustomTags"] = each_CustomTags;
                Json::Value each_StatisticName; ToJsonUtilS(StatisticName, each_StatisticName); output["StatisticName"] = each_StatisticName;
                Json::Value each_VersionChangeInterval; ToJsonUtilE(VersionChangeInterval, each_VersionChangeInterval); output["VersionChangeInterval"] = each_VersionChangeInterval;
                return output;
            }
        };

        struct PlayerStatisticDefinition : public PlayFabBaseModel
        {
            Boxed<StatisticAggregationMethod> AggregationMethod;
            Uint32 CurrentVersion;
            std::string StatisticName;
            Boxed<StatisticResetIntervalOption> VersionChangeInterval;

            PlayerStatisticDefinition() :
                PlayFabBaseModel(),
                AggregationMethod(),
                CurrentVersion(),
                StatisticName(),
                VersionChangeInterval()
            {}

            PlayerStatisticDefinition(const PlayerStatisticDefinition& src) :
                PlayFabBaseModel(),
                AggregationMethod(src.AggregationMethod),
                CurrentVersion(src.CurrentVersion),
                StatisticName(src.StatisticName),
                VersionChangeInterval(src.VersionChangeInterval)
            {}

            ~PlayerStatisticDefinition() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilE(input["AggregationMethod"], AggregationMethod);
                FromJsonUtilP(input["CurrentVersion"], CurrentVersion);
                FromJsonUtilS(input["StatisticName"], StatisticName);
                FromJsonUtilE(input["VersionChangeInterval"], VersionChangeInterval);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_AggregationMethod; ToJsonUtilE(AggregationMethod, each_AggregationMethod); output["AggregationMethod"] = each_AggregationMethod;
                Json::Value each_CurrentVersion; ToJsonUtilP(CurrentVersion, each_CurrentVersion); output["CurrentVersion"] = each_CurrentVersion;
                Json::Value each_StatisticName; ToJsonUtilS(StatisticName, each_StatisticName); output["StatisticName"] = each_StatisticName;
                Json::Value each_VersionChangeInterval; ToJsonUtilE(VersionChangeInterval, each_VersionChangeInterval); output["VersionChangeInterval"] = each_VersionChangeInterval;
                return output;
            }
        };

        struct CreatePlayerStatisticDefinitionResult : public PlayFabResultCommon
        {
            Boxed<PlayerStatisticDefinition> Statistic;

            CreatePlayerStatisticDefinitionResult() :
                PlayFabResultCommon(),
                Statistic()
            {}

            CreatePlayerStatisticDefinitionResult(const CreatePlayerStatisticDefinitionResult& src) :
                PlayFabResultCommon(),
                Statistic(src.Statistic)
            {}

            ~CreatePlayerStatisticDefinitionResult() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilO(input["Statistic"], Statistic);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_Statistic; ToJsonUtilO(Statistic, each_Statistic); output["Statistic"] = each_Statistic;
                return output;
            }
        };

        struct CreateTaskResult : public PlayFabResultCommon
        {
            std::string TaskId;

            CreateTaskResult() :
                PlayFabResultCommon(),
                TaskId()
            {}

            CreateTaskResult(const CreateTaskResult& src) :
                PlayFabResultCommon(),
                TaskId(src.TaskId)
            {}

            ~CreateTaskResult() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["TaskId"], TaskId);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_TaskId; ToJsonUtilS(TaskId, each_TaskId); output["TaskId"] = each_TaskId;
                return output;
            }
        };

        struct DeleteContentRequest : public PlayFabRequestCommon
        {
            std::string Key;

            DeleteContentRequest() :
                PlayFabRequestCommon(),
                Key()
            {}

            DeleteContentRequest(const DeleteContentRequest& src) :
                PlayFabRequestCommon(),
                Key(src.Key)
            {}

            ~DeleteContentRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["Key"], Key);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_Key; ToJsonUtilS(Key, each_Key); output["Key"] = each_Key;
                return output;
            }
        };

        struct DeleteMasterPlayerAccountRequest : public PlayFabRequestCommon
        {
            std::string MetaData;
            std::string PlayFabId;

            DeleteMasterPlayerAccountRequest() :
                PlayFabRequestCommon(),
                MetaData(),
                PlayFabId()
            {}

            DeleteMasterPlayerAccountRequest(const DeleteMasterPlayerAccountRequest& src) :
                PlayFabRequestCommon(),
                MetaData(src.MetaData),
                PlayFabId(src.PlayFabId)
            {}

            ~DeleteMasterPlayerAccountRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["MetaData"], MetaData);
                FromJsonUtilS(input["PlayFabId"], PlayFabId);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_MetaData; ToJsonUtilS(MetaData, each_MetaData); output["MetaData"] = each_MetaData;
                Json::Value each_PlayFabId; ToJsonUtilS(PlayFabId, each_PlayFabId); output["PlayFabId"] = each_PlayFabId;
                return output;
            }
        };

        struct DeleteMasterPlayerAccountResult : public PlayFabResultCommon
        {
            std::string JobReceiptId;
            std::list<std::string> TitleIds;

            DeleteMasterPlayerAccountResult() :
                PlayFabResultCommon(),
                JobReceiptId(),
                TitleIds()
            {}

            DeleteMasterPlayerAccountResult(const DeleteMasterPlayerAccountResult& src) :
                PlayFabResultCommon(),
                JobReceiptId(src.JobReceiptId),
                TitleIds(src.TitleIds)
            {}

            ~DeleteMasterPlayerAccountResult() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["JobReceiptId"], JobReceiptId);
                FromJsonUtilS(input["TitleIds"], TitleIds);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_JobReceiptId; ToJsonUtilS(JobReceiptId, each_JobReceiptId); output["JobReceiptId"] = each_JobReceiptId;
                Json::Value each_TitleIds; ToJsonUtilS(TitleIds, each_TitleIds); output["TitleIds"] = each_TitleIds;
                return output;
            }
        };

        struct DeleteOpenIdConnectionRequest : public PlayFabRequestCommon
        {
            std::string ConnectionId;

            DeleteOpenIdConnectionRequest() :
                PlayFabRequestCommon(),
                ConnectionId()
            {}

            DeleteOpenIdConnectionRequest(const DeleteOpenIdConnectionRequest& src) :
                PlayFabRequestCommon(),
                ConnectionId(src.ConnectionId)
            {}

            ~DeleteOpenIdConnectionRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["ConnectionId"], ConnectionId);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_ConnectionId; ToJsonUtilS(ConnectionId, each_ConnectionId); output["ConnectionId"] = each_ConnectionId;
                return output;
            }
        };

        struct DeletePlayerRequest : public PlayFabRequestCommon
        {
            std::string PlayFabId;

            DeletePlayerRequest() :
                PlayFabRequestCommon(),
                PlayFabId()
            {}

            DeletePlayerRequest(const DeletePlayerRequest& src) :
                PlayFabRequestCommon(),
                PlayFabId(src.PlayFabId)
            {}

            ~DeletePlayerRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["PlayFabId"], PlayFabId);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_PlayFabId; ToJsonUtilS(PlayFabId, each_PlayFabId); output["PlayFabId"] = each_PlayFabId;
                return output;
            }
        };

        struct DeletePlayerResult : public PlayFabResultCommon
        {

            DeletePlayerResult() :
                PlayFabResultCommon()
            {}

            DeletePlayerResult(const DeletePlayerResult&) :
                PlayFabResultCommon()
            {}

            ~DeletePlayerResult() = default;

            void FromJson(const Json::Value&) override
            {
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                return output;
            }
        };

        struct DeletePlayerSharedSecretRequest : public PlayFabRequestCommon
        {
            std::string SecretKey;

            DeletePlayerSharedSecretRequest() :
                PlayFabRequestCommon(),
                SecretKey()
            {}

            DeletePlayerSharedSecretRequest(const DeletePlayerSharedSecretRequest& src) :
                PlayFabRequestCommon(),
                SecretKey(src.SecretKey)
            {}

            ~DeletePlayerSharedSecretRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["SecretKey"], SecretKey);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_SecretKey; ToJsonUtilS(SecretKey, each_SecretKey); output["SecretKey"] = each_SecretKey;
                return output;
            }
        };

        struct DeletePlayerSharedSecretResult : public PlayFabResultCommon
        {

            DeletePlayerSharedSecretResult() :
                PlayFabResultCommon()
            {}

            DeletePlayerSharedSecretResult(const DeletePlayerSharedSecretResult&) :
                PlayFabResultCommon()
            {}

            ~DeletePlayerSharedSecretResult() = default;

            void FromJson(const Json::Value&) override
            {
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                return output;
            }
        };

        struct DeleteStoreRequest : public PlayFabRequestCommon
        {
            std::string CatalogVersion;
            std::map<std::string, std::string> CustomTags;
            std::string StoreId;

            DeleteStoreRequest() :
                PlayFabRequestCommon(),
                CatalogVersion(),
                CustomTags(),
                StoreId()
            {}

            DeleteStoreRequest(const DeleteStoreRequest& src) :
                PlayFabRequestCommon(),
                CatalogVersion(src.CatalogVersion),
                CustomTags(src.CustomTags),
                StoreId(src.StoreId)
            {}

            ~DeleteStoreRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["CatalogVersion"], CatalogVersion);
                FromJsonUtilS(input["CustomTags"], CustomTags);
                FromJsonUtilS(input["StoreId"], StoreId);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_CatalogVersion; ToJsonUtilS(CatalogVersion, each_CatalogVersion); output["CatalogVersion"] = each_CatalogVersion;
                Json::Value each_CustomTags; ToJsonUtilS(CustomTags, each_CustomTags); output["CustomTags"] = each_CustomTags;
                Json::Value each_StoreId; ToJsonUtilS(StoreId, each_StoreId); output["StoreId"] = each_StoreId;
                return output;
            }
        };

        struct DeleteStoreResult : public PlayFabResultCommon
        {

            DeleteStoreResult() :
                PlayFabResultCommon()
            {}

            DeleteStoreResult(const DeleteStoreResult&) :
                PlayFabResultCommon()
            {}

            ~DeleteStoreResult() = default;

            void FromJson(const Json::Value&) override
            {
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                return output;
            }
        };

        struct DeleteTaskRequest : public PlayFabRequestCommon
        {
            Boxed<NameIdentifier> Identifier;

            DeleteTaskRequest() :
                PlayFabRequestCommon(),
                Identifier()
            {}

            DeleteTaskRequest(const DeleteTaskRequest& src) :
                PlayFabRequestCommon(),
                Identifier(src.Identifier)
            {}

            ~DeleteTaskRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilO(input["Identifier"], Identifier);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_Identifier; ToJsonUtilO(Identifier, each_Identifier); output["Identifier"] = each_Identifier;
                return output;
            }
        };

        struct DeleteTitleDataOverrideRequest : public PlayFabRequestCommon
        {
            std::string OverrideLabel;

            DeleteTitleDataOverrideRequest() :
                PlayFabRequestCommon(),
                OverrideLabel()
            {}

            DeleteTitleDataOverrideRequest(const DeleteTitleDataOverrideRequest& src) :
                PlayFabRequestCommon(),
                OverrideLabel(src.OverrideLabel)
            {}

            ~DeleteTitleDataOverrideRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["OverrideLabel"], OverrideLabel);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_OverrideLabel; ToJsonUtilS(OverrideLabel, each_OverrideLabel); output["OverrideLabel"] = each_OverrideLabel;
                return output;
            }
        };

        struct DeleteTitleDataOverrideResult : public PlayFabResultCommon
        {

            DeleteTitleDataOverrideResult() :
                PlayFabResultCommon()
            {}

            DeleteTitleDataOverrideResult(const DeleteTitleDataOverrideResult&) :
                PlayFabResultCommon()
            {}

            ~DeleteTitleDataOverrideResult() = default;

            void FromJson(const Json::Value&) override
            {
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                return output;
            }
        };

        struct DeleteTitleRequest : public PlayFabRequestCommon
        {

            DeleteTitleRequest() :
                PlayFabRequestCommon()
            {}

            DeleteTitleRequest(const DeleteTitleRequest&) :
                PlayFabRequestCommon()
            {}

            ~DeleteTitleRequest() = default;

            void FromJson(const Json::Value&) override
            {
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                return output;
            }
        };

        struct DeleteTitleResult : public PlayFabResultCommon
        {

            DeleteTitleResult() :
                PlayFabResultCommon()
            {}

            DeleteTitleResult(const DeleteTitleResult&) :
                PlayFabResultCommon()
            {}

            ~DeleteTitleResult() = default;

            void FromJson(const Json::Value&) override
            {
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                return output;
            }
        };

        struct EmptyResponse : public PlayFabResultCommon
        {

            EmptyResponse() :
                PlayFabResultCommon()
            {}

            EmptyResponse(const EmptyResponse&) :
                PlayFabResultCommon()
            {}

            ~EmptyResponse() = default;

            void FromJson(const Json::Value&) override
            {
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                return output;
            }
        };

        struct EntityKey : public PlayFabBaseModel
        {
            std::string Id;
            std::string Type;

            EntityKey() :
                PlayFabBaseModel(),
                Id(),
                Type()
            {}

            EntityKey(const EntityKey& src) :
                PlayFabBaseModel(),
                Id(src.Id),
                Type(src.Type)
            {}

            ~EntityKey() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["Id"], Id);
                FromJsonUtilS(input["Type"], Type);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_Id; ToJsonUtilS(Id, each_Id); output["Id"] = each_Id;
                Json::Value each_Type; ToJsonUtilS(Type, each_Type); output["Type"] = each_Type;
                return output;
            }
        };

        struct ExportMasterPlayerDataRequest : public PlayFabRequestCommon
        {
            std::string PlayFabId;

            ExportMasterPlayerDataRequest() :
                PlayFabRequestCommon(),
                PlayFabId()
            {}

            ExportMasterPlayerDataRequest(const ExportMasterPlayerDataRequest& src) :
                PlayFabRequestCommon(),
                PlayFabId(src.PlayFabId)
            {}

            ~ExportMasterPlayerDataRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["PlayFabId"], PlayFabId);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_PlayFabId; ToJsonUtilS(PlayFabId, each_PlayFabId); output["PlayFabId"] = each_PlayFabId;
                return output;
            }
        };

        struct ExportMasterPlayerDataResult : public PlayFabResultCommon
        {
            std::string JobReceiptId;

            ExportMasterPlayerDataResult() :
                PlayFabResultCommon(),
                JobReceiptId()
            {}

            ExportMasterPlayerDataResult(const ExportMasterPlayerDataResult& src) :
                PlayFabResultCommon(),
                JobReceiptId(src.JobReceiptId)
            {}

            ~ExportMasterPlayerDataResult() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["JobReceiptId"], JobReceiptId);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_JobReceiptId; ToJsonUtilS(JobReceiptId, each_JobReceiptId); output["JobReceiptId"] = each_JobReceiptId;
                return output;
            }
        };

        struct GameModeInfo : public PlayFabBaseModel
        {
            std::string Gamemode;
            Uint32 MaxPlayerCount;
            Uint32 MinPlayerCount;
            Boxed<bool> StartOpen;

            GameModeInfo() :
                PlayFabBaseModel(),
                Gamemode(),
                MaxPlayerCount(),
                MinPlayerCount(),
                StartOpen()
            {}

            GameModeInfo(const GameModeInfo& src) :
                PlayFabBaseModel(),
                Gamemode(src.Gamemode),
                MaxPlayerCount(src.MaxPlayerCount),
                MinPlayerCount(src.MinPlayerCount),
                StartOpen(src.StartOpen)
            {}

            ~GameModeInfo() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["Gamemode"], Gamemode);
                FromJsonUtilP(input["MaxPlayerCount"], MaxPlayerCount);
                FromJsonUtilP(input["MinPlayerCount"], MinPlayerCount);
                FromJsonUtilP(input["StartOpen"], StartOpen);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_Gamemode; ToJsonUtilS(Gamemode, each_Gamemode); output["Gamemode"] = each_Gamemode;
                Json::Value each_MaxPlayerCount; ToJsonUtilP(MaxPlayerCount, each_MaxPlayerCount); output["MaxPlayerCount"] = each_MaxPlayerCount;
                Json::Value each_MinPlayerCount; ToJsonUtilP(MinPlayerCount, each_MinPlayerCount); output["MinPlayerCount"] = each_MinPlayerCount;
                Json::Value each_StartOpen; ToJsonUtilP(StartOpen, each_StartOpen); output["StartOpen"] = each_StartOpen;
                return output;
            }
        };

        struct GetActionsOnPlayersInSegmentTaskInstanceResult : public PlayFabResultCommon
        {
            Boxed<ActionsOnPlayersInSegmentTaskParameter> Parameter;
            Boxed<ActionsOnPlayersInSegmentTaskSummary> Summary;

            GetActionsOnPlayersInSegmentTaskInstanceResult() :
                PlayFabResultCommon(),
                Parameter(),
                Summary()
            {}

            GetActionsOnPlayersInSegmentTaskInstanceResult(const GetActionsOnPlayersInSegmentTaskInstanceResult& src) :
                PlayFabResultCommon(),
                Parameter(src.Parameter),
                Summary(src.Summary)
            {}

            ~GetActionsOnPlayersInSegmentTaskInstanceResult() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilO(input["Parameter"], Parameter);
                FromJsonUtilO(input["Summary"], Summary);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_Parameter; ToJsonUtilO(Parameter, each_Parameter); output["Parameter"] = each_Parameter;
                Json::Value each_Summary; ToJsonUtilO(Summary, each_Summary); output["Summary"] = each_Summary;
                return output;
            }
        };

        struct GetAllSegmentsRequest : public PlayFabRequestCommon
        {

            GetAllSegmentsRequest() :
                PlayFabRequestCommon()
            {}

            GetAllSegmentsRequest(const GetAllSegmentsRequest&) :
                PlayFabRequestCommon()
            {}

            ~GetAllSegmentsRequest() = default;

            void FromJson(const Json::Value&) override
            {
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                return output;
            }
        };

        struct GetSegmentResult : public PlayFabResultCommon
        {
            std::string ABTestParent;
            std::string Id;
            std::string Name;

            GetSegmentResult() :
                PlayFabResultCommon(),
                ABTestParent(),
                Id(),
                Name()
            {}

            GetSegmentResult(const GetSegmentResult& src) :
                PlayFabResultCommon(),
                ABTestParent(src.ABTestParent),
                Id(src.Id),
                Name(src.Name)
            {}

            ~GetSegmentResult() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["ABTestParent"], ABTestParent);
                FromJsonUtilS(input["Id"], Id);
                FromJsonUtilS(input["Name"], Name);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_ABTestParent; ToJsonUtilS(ABTestParent, each_ABTestParent); output["ABTestParent"] = each_ABTestParent;
                Json::Value each_Id; ToJsonUtilS(Id, each_Id); output["Id"] = each_Id;
                Json::Value each_Name; ToJsonUtilS(Name, each_Name); output["Name"] = each_Name;
                return output;
            }
        };

        struct GetAllSegmentsResult : public PlayFabResultCommon
        {
            std::list<GetSegmentResult> Segments;

            GetAllSegmentsResult() :
                PlayFabResultCommon(),
                Segments()
            {}

            GetAllSegmentsResult(const GetAllSegmentsResult& src) :
                PlayFabResultCommon(),
                Segments(src.Segments)
            {}

            ~GetAllSegmentsResult() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilO(input["Segments"], Segments);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_Segments; ToJsonUtilO(Segments, each_Segments); output["Segments"] = each_Segments;
                return output;
            }
        };

        struct GetCatalogItemsRequest : public PlayFabRequestCommon
        {
            std::string CatalogVersion;

            GetCatalogItemsRequest() :
                PlayFabRequestCommon(),
                CatalogVersion()
            {}

            GetCatalogItemsRequest(const GetCatalogItemsRequest& src) :
                PlayFabRequestCommon(),
                CatalogVersion(src.CatalogVersion)
            {}

            ~GetCatalogItemsRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["CatalogVersion"], CatalogVersion);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_CatalogVersion; ToJsonUtilS(CatalogVersion, each_CatalogVersion); output["CatalogVersion"] = each_CatalogVersion;
                return output;
            }
        };

        struct GetCatalogItemsResult : public PlayFabResultCommon
        {
            std::list<CatalogItem> Catalog;

            GetCatalogItemsResult() :
                PlayFabResultCommon(),
                Catalog()
            {}

            GetCatalogItemsResult(const GetCatalogItemsResult& src) :
                PlayFabResultCommon(),
                Catalog(src.Catalog)
            {}

            ~GetCatalogItemsResult() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilO(input["Catalog"], Catalog);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_Catalog; ToJsonUtilO(Catalog, each_Catalog); output["Catalog"] = each_Catalog;
                return output;
            }
        };

        struct GetCloudScriptRevisionRequest : public PlayFabRequestCommon
        {
            Boxed<Int32> Revision;
            Boxed<Int32> Version;

            GetCloudScriptRevisionRequest() :
                PlayFabRequestCommon(),
                Revision(),
                Version()
            {}

            GetCloudScriptRevisionRequest(const GetCloudScriptRevisionRequest& src) :
                PlayFabRequestCommon(),
                Revision(src.Revision),
                Version(src.Version)
            {}

            ~GetCloudScriptRevisionRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilP(input["Revision"], Revision);
                FromJsonUtilP(input["Version"], Version);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_Revision; ToJsonUtilP(Revision, each_Revision); output["Revision"] = each_Revision;
                Json::Value each_Version; ToJsonUtilP(Version, each_Version); output["Version"] = each_Version;
                return output;
            }
        };

        struct GetCloudScriptRevisionResult : public PlayFabResultCommon
        {
            time_t CreatedAt;
            std::list<CloudScriptFile> Files;
            bool IsPublished;
            Int32 Revision;
            Int32 Version;

            GetCloudScriptRevisionResult() :
                PlayFabResultCommon(),
                CreatedAt(),
                Files(),
                IsPublished(),
                Revision(),
                Version()
            {}

            GetCloudScriptRevisionResult(const GetCloudScriptRevisionResult& src) :
                PlayFabResultCommon(),
                CreatedAt(src.CreatedAt),
                Files(src.Files),
                IsPublished(src.IsPublished),
                Revision(src.Revision),
                Version(src.Version)
            {}

            ~GetCloudScriptRevisionResult() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilT(input["CreatedAt"], CreatedAt);
                FromJsonUtilO(input["Files"], Files);
                FromJsonUtilP(input["IsPublished"], IsPublished);
                FromJsonUtilP(input["Revision"], Revision);
                FromJsonUtilP(input["Version"], Version);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_CreatedAt; ToJsonUtilT(CreatedAt, each_CreatedAt); output["CreatedAt"] = each_CreatedAt;
                Json::Value each_Files; ToJsonUtilO(Files, each_Files); output["Files"] = each_Files;
                Json::Value each_IsPublished; ToJsonUtilP(IsPublished, each_IsPublished); output["IsPublished"] = each_IsPublished;
                Json::Value each_Revision; ToJsonUtilP(Revision, each_Revision); output["Revision"] = each_Revision;
                Json::Value each_Version; ToJsonUtilP(Version, each_Version); output["Version"] = each_Version;
                return output;
            }
        };

        struct GetCloudScriptTaskInstanceResult : public PlayFabResultCommon
        {
            Boxed<CloudScriptTaskParameter> Parameter;
            Boxed<CloudScriptTaskSummary> Summary;

            GetCloudScriptTaskInstanceResult() :
                PlayFabResultCommon(),
                Parameter(),
                Summary()
            {}

            GetCloudScriptTaskInstanceResult(const GetCloudScriptTaskInstanceResult& src) :
                PlayFabResultCommon(),
                Parameter(src.Parameter),
                Summary(src.Summary)
            {}

            ~GetCloudScriptTaskInstanceResult() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilO(input["Parameter"], Parameter);
                FromJsonUtilO(input["Summary"], Summary);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_Parameter; ToJsonUtilO(Parameter, each_Parameter); output["Parameter"] = each_Parameter;
                Json::Value each_Summary; ToJsonUtilO(Summary, each_Summary); output["Summary"] = each_Summary;
                return output;
            }
        };

        struct GetCloudScriptVersionsRequest : public PlayFabRequestCommon
        {

            GetCloudScriptVersionsRequest() :
                PlayFabRequestCommon()
            {}

            GetCloudScriptVersionsRequest(const GetCloudScriptVersionsRequest&) :
                PlayFabRequestCommon()
            {}

            ~GetCloudScriptVersionsRequest() = default;

            void FromJson(const Json::Value&) override
            {
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                return output;
            }
        };

        struct GetCloudScriptVersionsResult : public PlayFabResultCommon
        {
            std::list<CloudScriptVersionStatus> Versions;

            GetCloudScriptVersionsResult() :
                PlayFabResultCommon(),
                Versions()
            {}

            GetCloudScriptVersionsResult(const GetCloudScriptVersionsResult& src) :
                PlayFabResultCommon(),
                Versions(src.Versions)
            {}

            ~GetCloudScriptVersionsResult() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilO(input["Versions"], Versions);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_Versions; ToJsonUtilO(Versions, each_Versions); output["Versions"] = each_Versions;
                return output;
            }
        };

        struct GetContentListRequest : public PlayFabRequestCommon
        {
            std::string Prefix;

            GetContentListRequest() :
                PlayFabRequestCommon(),
                Prefix()
            {}

            GetContentListRequest(const GetContentListRequest& src) :
                PlayFabRequestCommon(),
                Prefix(src.Prefix)
            {}

            ~GetContentListRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["Prefix"], Prefix);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_Prefix; ToJsonUtilS(Prefix, each_Prefix); output["Prefix"] = each_Prefix;
                return output;
            }
        };

        struct GetContentListResult : public PlayFabResultCommon
        {
            std::list<ContentInfo> Contents;
            Int32 ItemCount;
            Uint32 TotalSize;

            GetContentListResult() :
                PlayFabResultCommon(),
                Contents(),
                ItemCount(),
                TotalSize()
            {}

            GetContentListResult(const GetContentListResult& src) :
                PlayFabResultCommon(),
                Contents(src.Contents),
                ItemCount(src.ItemCount),
                TotalSize(src.TotalSize)
            {}

            ~GetContentListResult() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilO(input["Contents"], Contents);
                FromJsonUtilP(input["ItemCount"], ItemCount);
                FromJsonUtilP(input["TotalSize"], TotalSize);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_Contents; ToJsonUtilO(Contents, each_Contents); output["Contents"] = each_Contents;
                Json::Value each_ItemCount; ToJsonUtilP(ItemCount, each_ItemCount); output["ItemCount"] = each_ItemCount;
                Json::Value each_TotalSize; ToJsonUtilP(TotalSize, each_TotalSize); output["TotalSize"] = each_TotalSize;
                return output;
            }
        };

        struct GetContentUploadUrlRequest : public PlayFabRequestCommon
        {
            std::string ContentType;
            std::string Key;

            GetContentUploadUrlRequest() :
                PlayFabRequestCommon(),
                ContentType(),
                Key()
            {}

            GetContentUploadUrlRequest(const GetContentUploadUrlRequest& src) :
                PlayFabRequestCommon(),
                ContentType(src.ContentType),
                Key(src.Key)
            {}

            ~GetContentUploadUrlRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["ContentType"], ContentType);
                FromJsonUtilS(input["Key"], Key);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_ContentType; ToJsonUtilS(ContentType, each_ContentType); output["ContentType"] = each_ContentType;
                Json::Value each_Key; ToJsonUtilS(Key, each_Key); output["Key"] = each_Key;
                return output;
            }
        };

        struct GetContentUploadUrlResult : public PlayFabResultCommon
        {
            std::string URL;

            GetContentUploadUrlResult() :
                PlayFabResultCommon(),
                URL()
            {}

            GetContentUploadUrlResult(const GetContentUploadUrlResult& src) :
                PlayFabResultCommon(),
                URL(src.URL)
            {}

            ~GetContentUploadUrlResult() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["URL"], URL);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_URL; ToJsonUtilS(URL, each_URL); output["URL"] = each_URL;
                return output;
            }
        };

        struct GetDataReportRequest : public PlayFabRequestCommon
        {
            Int32 Day;
            Int32 Month;
            std::string ReportName;
            Int32 Year;

            GetDataReportRequest() :
                PlayFabRequestCommon(),
                Day(),
                Month(),
                ReportName(),
                Year()
            {}

            GetDataReportRequest(const GetDataReportRequest& src) :
                PlayFabRequestCommon(),
                Day(src.Day),
                Month(src.Month),
                ReportName(src.ReportName),
                Year(src.Year)
            {}

            ~GetDataReportRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilP(input["Day"], Day);
                FromJsonUtilP(input["Month"], Month);
                FromJsonUtilS(input["ReportName"], ReportName);
                FromJsonUtilP(input["Year"], Year);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_Day; ToJsonUtilP(Day, each_Day); output["Day"] = each_Day;
                Json::Value each_Month; ToJsonUtilP(Month, each_Month); output["Month"] = each_Month;
                Json::Value each_ReportName; ToJsonUtilS(ReportName, each_ReportName); output["ReportName"] = each_ReportName;
                Json::Value each_Year; ToJsonUtilP(Year, each_Year); output["Year"] = each_Year;
                return output;
            }
        };

        struct GetDataReportResult : public PlayFabResultCommon
        {
            std::string DownloadUrl;

            GetDataReportResult() :
                PlayFabResultCommon(),
                DownloadUrl()
            {}

            GetDataReportResult(const GetDataReportResult& src) :
                PlayFabResultCommon(),
                DownloadUrl(src.DownloadUrl)
            {}

            ~GetDataReportResult() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["DownloadUrl"], DownloadUrl);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_DownloadUrl; ToJsonUtilS(DownloadUrl, each_DownloadUrl); output["DownloadUrl"] = each_DownloadUrl;
                return output;
            }
        };

        struct GetMatchmakerGameInfoRequest : public PlayFabRequestCommon
        {
            std::string LobbyId;

            GetMatchmakerGameInfoRequest() :
                PlayFabRequestCommon(),
                LobbyId()
            {}

            GetMatchmakerGameInfoRequest(const GetMatchmakerGameInfoRequest& src) :
                PlayFabRequestCommon(),
                LobbyId(src.LobbyId)
            {}

            ~GetMatchmakerGameInfoRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["LobbyId"], LobbyId);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_LobbyId; ToJsonUtilS(LobbyId, each_LobbyId); output["LobbyId"] = each_LobbyId;
                return output;
            }
        };

        struct GetMatchmakerGameInfoResult : public PlayFabResultCommon
        {
            std::string BuildVersion;
            Boxed<time_t> EndTime;
            std::string LobbyId;
            std::string Mode;
            std::list<std::string> Players;
            Boxed<Region> pfRegion;
            std::string ServerIPV4Address;
            std::string ServerIPV6Address;
            Uint32 ServerPort;
            std::string ServerPublicDNSName;
            time_t StartTime;
            std::string TitleId;

            GetMatchmakerGameInfoResult() :
                PlayFabResultCommon(),
                BuildVersion(),
                EndTime(),
                LobbyId(),
                Mode(),
                Players(),
                pfRegion(),
                ServerIPV4Address(),
                ServerIPV6Address(),
                ServerPort(),
                ServerPublicDNSName(),
                StartTime(),
                TitleId()
            {}

            GetMatchmakerGameInfoResult(const GetMatchmakerGameInfoResult& src) :
                PlayFabResultCommon(),
                BuildVersion(src.BuildVersion),
                EndTime(src.EndTime),
                LobbyId(src.LobbyId),
                Mode(src.Mode),
                Players(src.Players),
                pfRegion(src.pfRegion),
                ServerIPV4Address(src.ServerIPV4Address),
                ServerIPV6Address(src.ServerIPV6Address),
                ServerPort(src.ServerPort),
                ServerPublicDNSName(src.ServerPublicDNSName),
                StartTime(src.StartTime),
                TitleId(src.TitleId)
            {}

            ~GetMatchmakerGameInfoResult() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["BuildVersion"], BuildVersion);
                FromJsonUtilT(input["EndTime"], EndTime);
                FromJsonUtilS(input["LobbyId"], LobbyId);
                FromJsonUtilS(input["Mode"], Mode);
                FromJsonUtilS(input["Players"], Players);
                FromJsonUtilE(input["Region"], pfRegion);
                FromJsonUtilS(input["ServerIPV4Address"], ServerIPV4Address);
                FromJsonUtilS(input["ServerIPV6Address"], ServerIPV6Address);
                FromJsonUtilP(input["ServerPort"], ServerPort);
                FromJsonUtilS(input["ServerPublicDNSName"], ServerPublicDNSName);
                FromJsonUtilT(input["StartTime"], StartTime);
                FromJsonUtilS(input["TitleId"], TitleId);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_BuildVersion; ToJsonUtilS(BuildVersion, each_BuildVersion); output["BuildVersion"] = each_BuildVersion;
                Json::Value each_EndTime; ToJsonUtilT(EndTime, each_EndTime); output["EndTime"] = each_EndTime;
                Json::Value each_LobbyId; ToJsonUtilS(LobbyId, each_LobbyId); output["LobbyId"] = each_LobbyId;
                Json::Value each_Mode; ToJsonUtilS(Mode, each_Mode); output["Mode"] = each_Mode;
                Json::Value each_Players; ToJsonUtilS(Players, each_Players); output["Players"] = each_Players;
                Json::Value each_pfRegion; ToJsonUtilE(pfRegion, each_pfRegion); output["Region"] = each_pfRegion;
                Json::Value each_ServerIPV4Address; ToJsonUtilS(ServerIPV4Address, each_ServerIPV4Address); output["ServerIPV4Address"] = each_ServerIPV4Address;
                Json::Value each_ServerIPV6Address; ToJsonUtilS(ServerIPV6Address, each_ServerIPV6Address); output["ServerIPV6Address"] = each_ServerIPV6Address;
                Json::Value each_ServerPort; ToJsonUtilP(ServerPort, each_ServerPort); output["ServerPort"] = each_ServerPort;
                Json::Value each_ServerPublicDNSName; ToJsonUtilS(ServerPublicDNSName, each_ServerPublicDNSName); output["ServerPublicDNSName"] = each_ServerPublicDNSName;
                Json::Value each_StartTime; ToJsonUtilT(StartTime, each_StartTime); output["StartTime"] = each_StartTime;
                Json::Value each_TitleId; ToJsonUtilS(TitleId, each_TitleId); output["TitleId"] = each_TitleId;
                return output;
            }
        };

        struct GetMatchmakerGameModesRequest : public PlayFabRequestCommon
        {
            std::string BuildVersion;

            GetMatchmakerGameModesRequest() :
                PlayFabRequestCommon(),
                BuildVersion()
            {}

            GetMatchmakerGameModesRequest(const GetMatchmakerGameModesRequest& src) :
                PlayFabRequestCommon(),
                BuildVersion(src.BuildVersion)
            {}

            ~GetMatchmakerGameModesRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["BuildVersion"], BuildVersion);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_BuildVersion; ToJsonUtilS(BuildVersion, each_BuildVersion); output["BuildVersion"] = each_BuildVersion;
                return output;
            }
        };

        struct GetMatchmakerGameModesResult : public PlayFabResultCommon
        {
            std::list<GameModeInfo> GameModes;

            GetMatchmakerGameModesResult() :
                PlayFabResultCommon(),
                GameModes()
            {}

            GetMatchmakerGameModesResult(const GetMatchmakerGameModesResult& src) :
                PlayFabResultCommon(),
                GameModes(src.GameModes)
            {}

            ~GetMatchmakerGameModesResult() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilO(input["GameModes"], GameModes);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_GameModes; ToJsonUtilO(GameModes, each_GameModes); output["GameModes"] = each_GameModes;
                return output;
            }
        };

        struct GetPlayedTitleListRequest : public PlayFabRequestCommon
        {
            std::string PlayFabId;

            GetPlayedTitleListRequest() :
                PlayFabRequestCommon(),
                PlayFabId()
            {}

            GetPlayedTitleListRequest(const GetPlayedTitleListRequest& src) :
                PlayFabRequestCommon(),
                PlayFabId(src.PlayFabId)
            {}

            ~GetPlayedTitleListRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["PlayFabId"], PlayFabId);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_PlayFabId; ToJsonUtilS(PlayFabId, each_PlayFabId); output["PlayFabId"] = each_PlayFabId;
                return output;
            }
        };

        struct GetPlayedTitleListResult : public PlayFabResultCommon
        {
            std::list<std::string> TitleIds;

            GetPlayedTitleListResult() :
                PlayFabResultCommon(),
                TitleIds()
            {}

            GetPlayedTitleListResult(const GetPlayedTitleListResult& src) :
                PlayFabResultCommon(),
                TitleIds(src.TitleIds)
            {}

            ~GetPlayedTitleListResult() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["TitleIds"], TitleIds);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_TitleIds; ToJsonUtilS(TitleIds, each_TitleIds); output["TitleIds"] = each_TitleIds;
                return output;
            }
        };

        struct GetPlayerIdFromAuthTokenRequest : public PlayFabRequestCommon
        {
            std::string Token;
            AuthTokenType TokenType;

            GetPlayerIdFromAuthTokenRequest() :
                PlayFabRequestCommon(),
                Token(),
                TokenType()
            {}

            GetPlayerIdFromAuthTokenRequest(const GetPlayerIdFromAuthTokenRequest& src) :
                PlayFabRequestCommon(),
                Token(src.Token),
                TokenType(src.TokenType)
            {}

            ~GetPlayerIdFromAuthTokenRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["Token"], Token);
                FromJsonEnum(input["TokenType"], TokenType);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_Token; ToJsonUtilS(Token, each_Token); output["Token"] = each_Token;
                Json::Value each_TokenType; ToJsonEnum(TokenType, each_TokenType); output["TokenType"] = each_TokenType;
                return output;
            }
        };

        struct GetPlayerIdFromAuthTokenResult : public PlayFabResultCommon
        {
            std::string PlayFabId;

            GetPlayerIdFromAuthTokenResult() :
                PlayFabResultCommon(),
                PlayFabId()
            {}

            GetPlayerIdFromAuthTokenResult(const GetPlayerIdFromAuthTokenResult& src) :
                PlayFabResultCommon(),
                PlayFabId(src.PlayFabId)
            {}

            ~GetPlayerIdFromAuthTokenResult() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["PlayFabId"], PlayFabId);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_PlayFabId; ToJsonUtilS(PlayFabId, each_PlayFabId); output["PlayFabId"] = each_PlayFabId;
                return output;
            }
        };

        struct PlayerProfileViewConstraints : public PlayFabBaseModel
        {
            bool ShowAvatarUrl;
            bool ShowBannedUntil;
            bool ShowCampaignAttributions;
            bool ShowContactEmailAddresses;
            bool ShowCreated;
            bool ShowDisplayName;
            bool ShowExperimentVariants;
            bool ShowLastLogin;
            bool ShowLinkedAccounts;
            bool ShowLocations;
            bool ShowMemberships;
            bool ShowOrigination;
            bool ShowPushNotificationRegistrations;
            bool ShowStatistics;
            bool ShowTags;
            bool ShowTotalValueToDateInUsd;
            bool ShowValuesToDate;

            PlayerProfileViewConstraints() :
                PlayFabBaseModel(),
                ShowAvatarUrl(),
                ShowBannedUntil(),
                ShowCampaignAttributions(),
                ShowContactEmailAddresses(),
                ShowCreated(),
                ShowDisplayName(),
                ShowExperimentVariants(),
                ShowLastLogin(),
                ShowLinkedAccounts(),
                ShowLocations(),
                ShowMemberships(),
                ShowOrigination(),
                ShowPushNotificationRegistrations(),
                ShowStatistics(),
                ShowTags(),
                ShowTotalValueToDateInUsd(),
                ShowValuesToDate()
            {}

            PlayerProfileViewConstraints(const PlayerProfileViewConstraints& src) :
                PlayFabBaseModel(),
                ShowAvatarUrl(src.ShowAvatarUrl),
                ShowBannedUntil(src.ShowBannedUntil),
                ShowCampaignAttributions(src.ShowCampaignAttributions),
                ShowContactEmailAddresses(src.ShowContactEmailAddresses),
                ShowCreated(src.ShowCreated),
                ShowDisplayName(src.ShowDisplayName),
                ShowExperimentVariants(src.ShowExperimentVariants),
                ShowLastLogin(src.ShowLastLogin),
                ShowLinkedAccounts(src.ShowLinkedAccounts),
                ShowLocations(src.ShowLocations),
                ShowMemberships(src.ShowMemberships),
                ShowOrigination(src.ShowOrigination),
                ShowPushNotificationRegistrations(src.ShowPushNotificationRegistrations),
                ShowStatistics(src.ShowStatistics),
                ShowTags(src.ShowTags),
                ShowTotalValueToDateInUsd(src.ShowTotalValueToDateInUsd),
                ShowValuesToDate(src.ShowValuesToDate)
            {}

            ~PlayerProfileViewConstraints() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilP(input["ShowAvatarUrl"], ShowAvatarUrl);
                FromJsonUtilP(input["ShowBannedUntil"], ShowBannedUntil);
                FromJsonUtilP(input["ShowCampaignAttributions"], ShowCampaignAttributions);
                FromJsonUtilP(input["ShowContactEmailAddresses"], ShowContactEmailAddresses);
                FromJsonUtilP(input["ShowCreated"], ShowCreated);
                FromJsonUtilP(input["ShowDisplayName"], ShowDisplayName);
                FromJsonUtilP(input["ShowExperimentVariants"], ShowExperimentVariants);
                FromJsonUtilP(input["ShowLastLogin"], ShowLastLogin);
                FromJsonUtilP(input["ShowLinkedAccounts"], ShowLinkedAccounts);
                FromJsonUtilP(input["ShowLocations"], ShowLocations);
                FromJsonUtilP(input["ShowMemberships"], ShowMemberships);
                FromJsonUtilP(input["ShowOrigination"], ShowOrigination);
                FromJsonUtilP(input["ShowPushNotificationRegistrations"], ShowPushNotificationRegistrations);
                FromJsonUtilP(input["ShowStatistics"], ShowStatistics);
                FromJsonUtilP(input["ShowTags"], ShowTags);
                FromJsonUtilP(input["ShowTotalValueToDateInUsd"], ShowTotalValueToDateInUsd);
                FromJsonUtilP(input["ShowValuesToDate"], ShowValuesToDate);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_ShowAvatarUrl; ToJsonUtilP(ShowAvatarUrl, each_ShowAvatarUrl); output["ShowAvatarUrl"] = each_ShowAvatarUrl;
                Json::Value each_ShowBannedUntil; ToJsonUtilP(ShowBannedUntil, each_ShowBannedUntil); output["ShowBannedUntil"] = each_ShowBannedUntil;
                Json::Value each_ShowCampaignAttributions; ToJsonUtilP(ShowCampaignAttributions, each_ShowCampaignAttributions); output["ShowCampaignAttributions"] = each_ShowCampaignAttributions;
                Json::Value each_ShowContactEmailAddresses; ToJsonUtilP(ShowContactEmailAddresses, each_ShowContactEmailAddresses); output["ShowContactEmailAddresses"] = each_ShowContactEmailAddresses;
                Json::Value each_ShowCreated; ToJsonUtilP(ShowCreated, each_ShowCreated); output["ShowCreated"] = each_ShowCreated;
                Json::Value each_ShowDisplayName; ToJsonUtilP(ShowDisplayName, each_ShowDisplayName); output["ShowDisplayName"] = each_ShowDisplayName;
                Json::Value each_ShowExperimentVariants; ToJsonUtilP(ShowExperimentVariants, each_ShowExperimentVariants); output["ShowExperimentVariants"] = each_ShowExperimentVariants;
                Json::Value each_ShowLastLogin; ToJsonUtilP(ShowLastLogin, each_ShowLastLogin); output["ShowLastLogin"] = each_ShowLastLogin;
                Json::Value each_ShowLinkedAccounts; ToJsonUtilP(ShowLinkedAccounts, each_ShowLinkedAccounts); output["ShowLinkedAccounts"] = each_ShowLinkedAccounts;
                Json::Value each_ShowLocations; ToJsonUtilP(ShowLocations, each_ShowLocations); output["ShowLocations"] = each_ShowLocations;
                Json::Value each_ShowMemberships; ToJsonUtilP(ShowMemberships, each_ShowMemberships); output["ShowMemberships"] = each_ShowMemberships;
                Json::Value each_ShowOrigination; ToJsonUtilP(ShowOrigination, each_ShowOrigination); output["ShowOrigination"] = each_ShowOrigination;
                Json::Value each_ShowPushNotificationRegistrations; ToJsonUtilP(ShowPushNotificationRegistrations, each_ShowPushNotificationRegistrations); output["ShowPushNotificationRegistrations"] = each_ShowPushNotificationRegistrations;
                Json::Value each_ShowStatistics; ToJsonUtilP(ShowStatistics, each_ShowStatistics); output["ShowStatistics"] = each_ShowStatistics;
                Json::Value each_ShowTags; ToJsonUtilP(ShowTags, each_ShowTags); output["ShowTags"] = each_ShowTags;
                Json::Value each_ShowTotalValueToDateInUsd; ToJsonUtilP(ShowTotalValueToDateInUsd, each_ShowTotalValueToDateInUsd); output["ShowTotalValueToDateInUsd"] = each_ShowTotalValueToDateInUsd;
                Json::Value each_ShowValuesToDate; ToJsonUtilP(ShowValuesToDate, each_ShowValuesToDate); output["ShowValuesToDate"] = each_ShowValuesToDate;
                return output;
            }
        };

        struct GetPlayerProfileRequest : public PlayFabRequestCommon
        {
            std::map<std::string, std::string> CustomTags;
            std::string PlayFabId;
            Boxed<PlayerProfileViewConstraints> ProfileConstraints;

            GetPlayerProfileRequest() :
                PlayFabRequestCommon(),
                CustomTags(),
                PlayFabId(),
                ProfileConstraints()
            {}

            GetPlayerProfileRequest(const GetPlayerProfileRequest& src) :
                PlayFabRequestCommon(),
                CustomTags(src.CustomTags),
                PlayFabId(src.PlayFabId),
                ProfileConstraints(src.ProfileConstraints)
            {}

            ~GetPlayerProfileRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["CustomTags"], CustomTags);
                FromJsonUtilS(input["PlayFabId"], PlayFabId);
                FromJsonUtilO(input["ProfileConstraints"], ProfileConstraints);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_CustomTags; ToJsonUtilS(CustomTags, each_CustomTags); output["CustomTags"] = each_CustomTags;
                Json::Value each_PlayFabId; ToJsonUtilS(PlayFabId, each_PlayFabId); output["PlayFabId"] = each_PlayFabId;
                Json::Value each_ProfileConstraints; ToJsonUtilO(ProfileConstraints, each_ProfileConstraints); output["ProfileConstraints"] = each_ProfileConstraints;
                return output;
            }
        };

        struct LinkedPlatformAccountModel : public PlayFabBaseModel
        {
            std::string Email;
            Boxed<LoginIdentityProvider> Platform;
            std::string PlatformUserId;
            std::string Username;

            LinkedPlatformAccountModel() :
                PlayFabBaseModel(),
                Email(),
                Platform(),
                PlatformUserId(),
                Username()
            {}

            LinkedPlatformAccountModel(const LinkedPlatformAccountModel& src) :
                PlayFabBaseModel(),
                Email(src.Email),
                Platform(src.Platform),
                PlatformUserId(src.PlatformUserId),
                Username(src.Username)
            {}

            ~LinkedPlatformAccountModel() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["Email"], Email);
                FromJsonUtilE(input["Platform"], Platform);
                FromJsonUtilS(input["PlatformUserId"], PlatformUserId);
                FromJsonUtilS(input["Username"], Username);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_Email; ToJsonUtilS(Email, each_Email); output["Email"] = each_Email;
                Json::Value each_Platform; ToJsonUtilE(Platform, each_Platform); output["Platform"] = each_Platform;
                Json::Value each_PlatformUserId; ToJsonUtilS(PlatformUserId, each_PlatformUserId); output["PlatformUserId"] = each_PlatformUserId;
                Json::Value each_Username; ToJsonUtilS(Username, each_Username); output["Username"] = each_Username;
                return output;
            }
        };

        struct LocationModel : public PlayFabBaseModel
        {
            std::string City;
            Boxed<ContinentCode> pfContinentCode;
            Boxed<CountryCode> pfCountryCode;
            Boxed<double> Latitude;
            Boxed<double> Longitude;

            LocationModel() :
                PlayFabBaseModel(),
                City(),
                pfContinentCode(),
                pfCountryCode(),
                Latitude(),
                Longitude()
            {}

            LocationModel(const LocationModel& src) :
                PlayFabBaseModel(),
                City(src.City),
                pfContinentCode(src.pfContinentCode),
                pfCountryCode(src.pfCountryCode),
                Latitude(src.Latitude),
                Longitude(src.Longitude)
            {}

            ~LocationModel() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["City"], City);
                FromJsonUtilE(input["ContinentCode"], pfContinentCode);
                FromJsonUtilE(input["CountryCode"], pfCountryCode);
                FromJsonUtilP(input["Latitude"], Latitude);
                FromJsonUtilP(input["Longitude"], Longitude);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_City; ToJsonUtilS(City, each_City); output["City"] = each_City;
                Json::Value each_pfContinentCode; ToJsonUtilE(pfContinentCode, each_pfContinentCode); output["ContinentCode"] = each_pfContinentCode;
                Json::Value each_pfCountryCode; ToJsonUtilE(pfCountryCode, each_pfCountryCode); output["CountryCode"] = each_pfCountryCode;
                Json::Value each_Latitude; ToJsonUtilP(Latitude, each_Latitude); output["Latitude"] = each_Latitude;
                Json::Value each_Longitude; ToJsonUtilP(Longitude, each_Longitude); output["Longitude"] = each_Longitude;
                return output;
            }
        };

        struct SubscriptionModel : public PlayFabBaseModel
        {
            time_t Expiration;
            time_t InitialSubscriptionTime;
            bool IsActive;
            Boxed<SubscriptionProviderStatus> Status;
            std::string SubscriptionId;
            std::string SubscriptionItemId;
            std::string SubscriptionProvider;

            SubscriptionModel() :
                PlayFabBaseModel(),
                Expiration(),
                InitialSubscriptionTime(),
                IsActive(),
                Status(),
                SubscriptionId(),
                SubscriptionItemId(),
                SubscriptionProvider()
            {}

            SubscriptionModel(const SubscriptionModel& src) :
                PlayFabBaseModel(),
                Expiration(src.Expiration),
                InitialSubscriptionTime(src.InitialSubscriptionTime),
                IsActive(src.IsActive),
                Status(src.Status),
                SubscriptionId(src.SubscriptionId),
                SubscriptionItemId(src.SubscriptionItemId),
                SubscriptionProvider(src.SubscriptionProvider)
            {}

            ~SubscriptionModel() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilT(input["Expiration"], Expiration);
                FromJsonUtilT(input["InitialSubscriptionTime"], InitialSubscriptionTime);
                FromJsonUtilP(input["IsActive"], IsActive);
                FromJsonUtilE(input["Status"], Status);
                FromJsonUtilS(input["SubscriptionId"], SubscriptionId);
                FromJsonUtilS(input["SubscriptionItemId"], SubscriptionItemId);
                FromJsonUtilS(input["SubscriptionProvider"], SubscriptionProvider);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_Expiration; ToJsonUtilT(Expiration, each_Expiration); output["Expiration"] = each_Expiration;
                Json::Value each_InitialSubscriptionTime; ToJsonUtilT(InitialSubscriptionTime, each_InitialSubscriptionTime); output["InitialSubscriptionTime"] = each_InitialSubscriptionTime;
                Json::Value each_IsActive; ToJsonUtilP(IsActive, each_IsActive); output["IsActive"] = each_IsActive;
                Json::Value each_Status; ToJsonUtilE(Status, each_Status); output["Status"] = each_Status;
                Json::Value each_SubscriptionId; ToJsonUtilS(SubscriptionId, each_SubscriptionId); output["SubscriptionId"] = each_SubscriptionId;
                Json::Value each_SubscriptionItemId; ToJsonUtilS(SubscriptionItemId, each_SubscriptionItemId); output["SubscriptionItemId"] = each_SubscriptionItemId;
                Json::Value each_SubscriptionProvider; ToJsonUtilS(SubscriptionProvider, each_SubscriptionProvider); output["SubscriptionProvider"] = each_SubscriptionProvider;
                return output;
            }
        };

        struct MembershipModel : public PlayFabBaseModel
        {
            bool IsActive;
            time_t MembershipExpiration;
            std::string MembershipId;
            Boxed<time_t> OverrideExpiration;
            std::list<SubscriptionModel> Subscriptions;

            MembershipModel() :
                PlayFabBaseModel(),
                IsActive(),
                MembershipExpiration(),
                MembershipId(),
                OverrideExpiration(),
                Subscriptions()
            {}

            MembershipModel(const MembershipModel& src) :
                PlayFabBaseModel(),
                IsActive(src.IsActive),
                MembershipExpiration(src.MembershipExpiration),
                MembershipId(src.MembershipId),
                OverrideExpiration(src.OverrideExpiration),
                Subscriptions(src.Subscriptions)
            {}

            ~MembershipModel() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilP(input["IsActive"], IsActive);
                FromJsonUtilT(input["MembershipExpiration"], MembershipExpiration);
                FromJsonUtilS(input["MembershipId"], MembershipId);
                FromJsonUtilT(input["OverrideExpiration"], OverrideExpiration);
                FromJsonUtilO(input["Subscriptions"], Subscriptions);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_IsActive; ToJsonUtilP(IsActive, each_IsActive); output["IsActive"] = each_IsActive;
                Json::Value each_MembershipExpiration; ToJsonUtilT(MembershipExpiration, each_MembershipExpiration); output["MembershipExpiration"] = each_MembershipExpiration;
                Json::Value each_MembershipId; ToJsonUtilS(MembershipId, each_MembershipId); output["MembershipId"] = each_MembershipId;
                Json::Value each_OverrideExpiration; ToJsonUtilT(OverrideExpiration, each_OverrideExpiration); output["OverrideExpiration"] = each_OverrideExpiration;
                Json::Value each_Subscriptions; ToJsonUtilO(Subscriptions, each_Subscriptions); output["Subscriptions"] = each_Subscriptions;
                return output;
            }
        };

        struct PushNotificationRegistrationModel : public PlayFabBaseModel
        {
            std::string NotificationEndpointARN;
            Boxed<PushNotificationPlatform> Platform;

            PushNotificationRegistrationModel() :
                PlayFabBaseModel(),
                NotificationEndpointARN(),
                Platform()
            {}

            PushNotificationRegistrationModel(const PushNotificationRegistrationModel& src) :
                PlayFabBaseModel(),
                NotificationEndpointARN(src.NotificationEndpointARN),
                Platform(src.Platform)
            {}

            ~PushNotificationRegistrationModel() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["NotificationEndpointARN"], NotificationEndpointARN);
                FromJsonUtilE(input["Platform"], Platform);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_NotificationEndpointARN; ToJsonUtilS(NotificationEndpointARN, each_NotificationEndpointARN); output["NotificationEndpointARN"] = each_NotificationEndpointARN;
                Json::Value each_Platform; ToJsonUtilE(Platform, each_Platform); output["Platform"] = each_Platform;
                return output;
            }
        };

        struct StatisticModel : public PlayFabBaseModel
        {
            std::string Name;
            Int32 Value;
            Int32 Version;

            StatisticModel() :
                PlayFabBaseModel(),
                Name(),
                Value(),
                Version()
            {}

            StatisticModel(const StatisticModel& src) :
                PlayFabBaseModel(),
                Name(src.Name),
                Value(src.Value),
                Version(src.Version)
            {}

            ~StatisticModel() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["Name"], Name);
                FromJsonUtilP(input["Value"], Value);
                FromJsonUtilP(input["Version"], Version);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_Name; ToJsonUtilS(Name, each_Name); output["Name"] = each_Name;
                Json::Value each_Value; ToJsonUtilP(Value, each_Value); output["Value"] = each_Value;
                Json::Value each_Version; ToJsonUtilP(Version, each_Version); output["Version"] = each_Version;
                return output;
            }
        };

        struct TagModel : public PlayFabBaseModel
        {
            std::string TagValue;

            TagModel() :
                PlayFabBaseModel(),
                TagValue()
            {}

            TagModel(const TagModel& src) :
                PlayFabBaseModel(),
                TagValue(src.TagValue)
            {}

            ~TagModel() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["TagValue"], TagValue);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_TagValue; ToJsonUtilS(TagValue, each_TagValue); output["TagValue"] = each_TagValue;
                return output;
            }
        };

        struct ValueToDateModel : public PlayFabBaseModel
        {
            std::string Currency;
            Uint32 TotalValue;
            std::string TotalValueAsDecimal;

            ValueToDateModel() :
                PlayFabBaseModel(),
                Currency(),
                TotalValue(),
                TotalValueAsDecimal()
            {}

            ValueToDateModel(const ValueToDateModel& src) :
                PlayFabBaseModel(),
                Currency(src.Currency),
                TotalValue(src.TotalValue),
                TotalValueAsDecimal(src.TotalValueAsDecimal)
            {}

            ~ValueToDateModel() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["Currency"], Currency);
                FromJsonUtilP(input["TotalValue"], TotalValue);
                FromJsonUtilS(input["TotalValueAsDecimal"], TotalValueAsDecimal);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_Currency; ToJsonUtilS(Currency, each_Currency); output["Currency"] = each_Currency;
                Json::Value each_TotalValue; ToJsonUtilP(TotalValue, each_TotalValue); output["TotalValue"] = each_TotalValue;
                Json::Value each_TotalValueAsDecimal; ToJsonUtilS(TotalValueAsDecimal, each_TotalValueAsDecimal); output["TotalValueAsDecimal"] = each_TotalValueAsDecimal;
                return output;
            }
        };

        struct PlayerProfileModel : public PlayFabBaseModel
        {
            std::list<AdCampaignAttributionModel> AdCampaignAttributions;
            std::string AvatarUrl;
            Boxed<time_t> BannedUntil;
            std::list<ContactEmailInfoModel> ContactEmailAddresses;
            Boxed<time_t> Created;
            std::string DisplayName;
            std::list<std::string> ExperimentVariants;
            Boxed<time_t> LastLogin;
            std::list<LinkedPlatformAccountModel> LinkedAccounts;
            std::list<LocationModel> Locations;
            std::list<MembershipModel> Memberships;
            Boxed<LoginIdentityProvider> Origination;
            std::string PlayerId;
            std::string PublisherId;
            std::list<PushNotificationRegistrationModel> PushNotificationRegistrations;
            std::list<StatisticModel> Statistics;
            std::list<TagModel> Tags;
            std::string TitleId;
            Boxed<Uint32> TotalValueToDateInUSD;
            std::list<ValueToDateModel> ValuesToDate;

            PlayerProfileModel() :
                PlayFabBaseModel(),
                AdCampaignAttributions(),
                AvatarUrl(),
                BannedUntil(),
                ContactEmailAddresses(),
                Created(),
                DisplayName(),
                ExperimentVariants(),
                LastLogin(),
                LinkedAccounts(),
                Locations(),
                Memberships(),
                Origination(),
                PlayerId(),
                PublisherId(),
                PushNotificationRegistrations(),
                Statistics(),
                Tags(),
                TitleId(),
                TotalValueToDateInUSD(),
                ValuesToDate()
            {}

            PlayerProfileModel(const PlayerProfileModel& src) :
                PlayFabBaseModel(),
                AdCampaignAttributions(src.AdCampaignAttributions),
                AvatarUrl(src.AvatarUrl),
                BannedUntil(src.BannedUntil),
                ContactEmailAddresses(src.ContactEmailAddresses),
                Created(src.Created),
                DisplayName(src.DisplayName),
                ExperimentVariants(src.ExperimentVariants),
                LastLogin(src.LastLogin),
                LinkedAccounts(src.LinkedAccounts),
                Locations(src.Locations),
                Memberships(src.Memberships),
                Origination(src.Origination),
                PlayerId(src.PlayerId),
                PublisherId(src.PublisherId),
                PushNotificationRegistrations(src.PushNotificationRegistrations),
                Statistics(src.Statistics),
                Tags(src.Tags),
                TitleId(src.TitleId),
                TotalValueToDateInUSD(src.TotalValueToDateInUSD),
                ValuesToDate(src.ValuesToDate)
            {}

            ~PlayerProfileModel() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilO(input["AdCampaignAttributions"], AdCampaignAttributions);
                FromJsonUtilS(input["AvatarUrl"], AvatarUrl);
                FromJsonUtilT(input["BannedUntil"], BannedUntil);
                FromJsonUtilO(input["ContactEmailAddresses"], ContactEmailAddresses);
                FromJsonUtilT(input["Created"], Created);
                FromJsonUtilS(input["DisplayName"], DisplayName);
                FromJsonUtilS(input["ExperimentVariants"], ExperimentVariants);
                FromJsonUtilT(input["LastLogin"], LastLogin);
                FromJsonUtilO(input["LinkedAccounts"], LinkedAccounts);
                FromJsonUtilO(input["Locations"], Locations);
                FromJsonUtilO(input["Memberships"], Memberships);
                FromJsonUtilE(input["Origination"], Origination);
                FromJsonUtilS(input["PlayerId"], PlayerId);
                FromJsonUtilS(input["PublisherId"], PublisherId);
                FromJsonUtilO(input["PushNotificationRegistrations"], PushNotificationRegistrations);
                FromJsonUtilO(input["Statistics"], Statistics);
                FromJsonUtilO(input["Tags"], Tags);
                FromJsonUtilS(input["TitleId"], TitleId);
                FromJsonUtilP(input["TotalValueToDateInUSD"], TotalValueToDateInUSD);
                FromJsonUtilO(input["ValuesToDate"], ValuesToDate);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_AdCampaignAttributions; ToJsonUtilO(AdCampaignAttributions, each_AdCampaignAttributions); output["AdCampaignAttributions"] = each_AdCampaignAttributions;
                Json::Value each_AvatarUrl; ToJsonUtilS(AvatarUrl, each_AvatarUrl); output["AvatarUrl"] = each_AvatarUrl;
                Json::Value each_BannedUntil; ToJsonUtilT(BannedUntil, each_BannedUntil); output["BannedUntil"] = each_BannedUntil;
                Json::Value each_ContactEmailAddresses; ToJsonUtilO(ContactEmailAddresses, each_ContactEmailAddresses); output["ContactEmailAddresses"] = each_ContactEmailAddresses;
                Json::Value each_Created; ToJsonUtilT(Created, each_Created); output["Created"] = each_Created;
                Json::Value each_DisplayName; ToJsonUtilS(DisplayName, each_DisplayName); output["DisplayName"] = each_DisplayName;
                Json::Value each_ExperimentVariants; ToJsonUtilS(ExperimentVariants, each_ExperimentVariants); output["ExperimentVariants"] = each_ExperimentVariants;
                Json::Value each_LastLogin; ToJsonUtilT(LastLogin, each_LastLogin); output["LastLogin"] = each_LastLogin;
                Json::Value each_LinkedAccounts; ToJsonUtilO(LinkedAccounts, each_LinkedAccounts); output["LinkedAccounts"] = each_LinkedAccounts;
                Json::Value each_Locations; ToJsonUtilO(Locations, each_Locations); output["Locations"] = each_Locations;
                Json::Value each_Memberships; ToJsonUtilO(Memberships, each_Memberships); output["Memberships"] = each_Memberships;
                Json::Value each_Origination; ToJsonUtilE(Origination, each_Origination); output["Origination"] = each_Origination;
                Json::Value each_PlayerId; ToJsonUtilS(PlayerId, each_PlayerId); output["PlayerId"] = each_PlayerId;
                Json::Value each_PublisherId; ToJsonUtilS(PublisherId, each_PublisherId); output["PublisherId"] = each_PublisherId;
                Json::Value each_PushNotificationRegistrations; ToJsonUtilO(PushNotificationRegistrations, each_PushNotificationRegistrations); output["PushNotificationRegistrations"] = each_PushNotificationRegistrations;
                Json::Value each_Statistics; ToJsonUtilO(Statistics, each_Statistics); output["Statistics"] = each_Statistics;
                Json::Value each_Tags; ToJsonUtilO(Tags, each_Tags); output["Tags"] = each_Tags;
                Json::Value each_TitleId; ToJsonUtilS(TitleId, each_TitleId); output["TitleId"] = each_TitleId;
                Json::Value each_TotalValueToDateInUSD; ToJsonUtilP(TotalValueToDateInUSD, each_TotalValueToDateInUSD); output["TotalValueToDateInUSD"] = each_TotalValueToDateInUSD;
                Json::Value each_ValuesToDate; ToJsonUtilO(ValuesToDate, each_ValuesToDate); output["ValuesToDate"] = each_ValuesToDate;
                return output;
            }
        };

        struct GetPlayerProfileResult : public PlayFabResultCommon
        {
            Boxed<PlayerProfileModel> PlayerProfile;

            GetPlayerProfileResult() :
                PlayFabResultCommon(),
                PlayerProfile()
            {}

            GetPlayerProfileResult(const GetPlayerProfileResult& src) :
                PlayFabResultCommon(),
                PlayerProfile(src.PlayerProfile)
            {}

            ~GetPlayerProfileResult() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilO(input["PlayerProfile"], PlayerProfile);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_PlayerProfile; ToJsonUtilO(PlayerProfile, each_PlayerProfile); output["PlayerProfile"] = each_PlayerProfile;
                return output;
            }
        };

        struct GetPlayerSegmentsResult : public PlayFabResultCommon
        {
            std::list<GetSegmentResult> Segments;

            GetPlayerSegmentsResult() :
                PlayFabResultCommon(),
                Segments()
            {}

            GetPlayerSegmentsResult(const GetPlayerSegmentsResult& src) :
                PlayFabResultCommon(),
                Segments(src.Segments)
            {}

            ~GetPlayerSegmentsResult() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilO(input["Segments"], Segments);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_Segments; ToJsonUtilO(Segments, each_Segments); output["Segments"] = each_Segments;
                return output;
            }
        };

        struct GetPlayerSharedSecretsRequest : public PlayFabRequestCommon
        {

            GetPlayerSharedSecretsRequest() :
                PlayFabRequestCommon()
            {}

            GetPlayerSharedSecretsRequest(const GetPlayerSharedSecretsRequest&) :
                PlayFabRequestCommon()
            {}

            ~GetPlayerSharedSecretsRequest() = default;

            void FromJson(const Json::Value&) override
            {
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                return output;
            }
        };

        struct SharedSecret : public PlayFabBaseModel
        {
            bool Disabled;
            std::string FriendlyName;
            std::string SecretKey;

            SharedSecret() :
                PlayFabBaseModel(),
                Disabled(),
                FriendlyName(),
                SecretKey()
            {}

            SharedSecret(const SharedSecret& src) :
                PlayFabBaseModel(),
                Disabled(src.Disabled),
                FriendlyName(src.FriendlyName),
                SecretKey(src.SecretKey)
            {}

            ~SharedSecret() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilP(input["Disabled"], Disabled);
                FromJsonUtilS(input["FriendlyName"], FriendlyName);
                FromJsonUtilS(input["SecretKey"], SecretKey);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_Disabled; ToJsonUtilP(Disabled, each_Disabled); output["Disabled"] = each_Disabled;
                Json::Value each_FriendlyName; ToJsonUtilS(FriendlyName, each_FriendlyName); output["FriendlyName"] = each_FriendlyName;
                Json::Value each_SecretKey; ToJsonUtilS(SecretKey, each_SecretKey); output["SecretKey"] = each_SecretKey;
                return output;
            }
        };

        struct GetPlayerSharedSecretsResult : public PlayFabResultCommon
        {
            std::list<SharedSecret> SharedSecrets;

            GetPlayerSharedSecretsResult() :
                PlayFabResultCommon(),
                SharedSecrets()
            {}

            GetPlayerSharedSecretsResult(const GetPlayerSharedSecretsResult& src) :
                PlayFabResultCommon(),
                SharedSecrets(src.SharedSecrets)
            {}

            ~GetPlayerSharedSecretsResult() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilO(input["SharedSecrets"], SharedSecrets);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_SharedSecrets; ToJsonUtilO(SharedSecrets, each_SharedSecrets); output["SharedSecrets"] = each_SharedSecrets;
                return output;
            }
        };

        struct GetPlayersInSegmentRequest : public PlayFabRequestCommon
        {
            std::string ContinuationToken;
            std::map<std::string, std::string> CustomTags;
            Boxed<Uint32> MaxBatchSize;
            Boxed<Uint32> SecondsToLive;
            std::string SegmentId;

            GetPlayersInSegmentRequest() :
                PlayFabRequestCommon(),
                ContinuationToken(),
                CustomTags(),
                MaxBatchSize(),
                SecondsToLive(),
                SegmentId()
            {}

            GetPlayersInSegmentRequest(const GetPlayersInSegmentRequest& src) :
                PlayFabRequestCommon(),
                ContinuationToken(src.ContinuationToken),
                CustomTags(src.CustomTags),
                MaxBatchSize(src.MaxBatchSize),
                SecondsToLive(src.SecondsToLive),
                SegmentId(src.SegmentId)
            {}

            ~GetPlayersInSegmentRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["ContinuationToken"], ContinuationToken);
                FromJsonUtilS(input["CustomTags"], CustomTags);
                FromJsonUtilP(input["MaxBatchSize"], MaxBatchSize);
                FromJsonUtilP(input["SecondsToLive"], SecondsToLive);
                FromJsonUtilS(input["SegmentId"], SegmentId);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_ContinuationToken; ToJsonUtilS(ContinuationToken, each_ContinuationToken); output["ContinuationToken"] = each_ContinuationToken;
                Json::Value each_CustomTags; ToJsonUtilS(CustomTags, each_CustomTags); output["CustomTags"] = each_CustomTags;
                Json::Value each_MaxBatchSize; ToJsonUtilP(MaxBatchSize, each_MaxBatchSize); output["MaxBatchSize"] = each_MaxBatchSize;
                Json::Value each_SecondsToLive; ToJsonUtilP(SecondsToLive, each_SecondsToLive); output["SecondsToLive"] = each_SecondsToLive;
                Json::Value each_SegmentId; ToJsonUtilS(SegmentId, each_SegmentId); output["SegmentId"] = each_SegmentId;
                return output;
            }
        };

        struct PlayerLinkedAccount : public PlayFabBaseModel
        {
            std::string Email;
            Boxed<LoginIdentityProvider> Platform;
            std::string PlatformUserId;
            std::string Username;

            PlayerLinkedAccount() :
                PlayFabBaseModel(),
                Email(),
                Platform(),
                PlatformUserId(),
                Username()
            {}

            PlayerLinkedAccount(const PlayerLinkedAccount& src) :
                PlayFabBaseModel(),
                Email(src.Email),
                Platform(src.Platform),
                PlatformUserId(src.PlatformUserId),
                Username(src.Username)
            {}

            ~PlayerLinkedAccount() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["Email"], Email);
                FromJsonUtilE(input["Platform"], Platform);
                FromJsonUtilS(input["PlatformUserId"], PlatformUserId);
                FromJsonUtilS(input["Username"], Username);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_Email; ToJsonUtilS(Email, each_Email); output["Email"] = each_Email;
                Json::Value each_Platform; ToJsonUtilE(Platform, each_Platform); output["Platform"] = each_Platform;
                Json::Value each_PlatformUserId; ToJsonUtilS(PlatformUserId, each_PlatformUserId); output["PlatformUserId"] = each_PlatformUserId;
                Json::Value each_Username; ToJsonUtilS(Username, each_Username); output["Username"] = each_Username;
                return output;
            }
        };

        struct PlayerLocation : public PlayFabBaseModel
        {
            std::string City;
            ContinentCode pfContinentCode;
            CountryCode pfCountryCode;
            Boxed<double> Latitude;
            Boxed<double> Longitude;

            PlayerLocation() :
                PlayFabBaseModel(),
                City(),
                pfContinentCode(),
                pfCountryCode(),
                Latitude(),
                Longitude()
            {}

            PlayerLocation(const PlayerLocation& src) :
                PlayFabBaseModel(),
                City(src.City),
                pfContinentCode(src.pfContinentCode),
                pfCountryCode(src.pfCountryCode),
                Latitude(src.Latitude),
                Longitude(src.Longitude)
            {}

            ~PlayerLocation() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["City"], City);
                FromJsonEnum(input["ContinentCode"], pfContinentCode);
                FromJsonEnum(input["CountryCode"], pfCountryCode);
                FromJsonUtilP(input["Latitude"], Latitude);
                FromJsonUtilP(input["Longitude"], Longitude);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_City; ToJsonUtilS(City, each_City); output["City"] = each_City;
                Json::Value each_pfContinentCode; ToJsonEnum(pfContinentCode, each_pfContinentCode); output["ContinentCode"] = each_pfContinentCode;
                Json::Value each_pfCountryCode; ToJsonEnum(pfCountryCode, each_pfCountryCode); output["CountryCode"] = each_pfCountryCode;
                Json::Value each_Latitude; ToJsonUtilP(Latitude, each_Latitude); output["Latitude"] = each_Latitude;
                Json::Value each_Longitude; ToJsonUtilP(Longitude, each_Longitude); output["Longitude"] = each_Longitude;
                return output;
            }
        };

        struct PlayerStatistic : public PlayFabBaseModel
        {
            std::string Id;
            std::string Name;
            Int32 StatisticValue;
            Int32 StatisticVersion;

            PlayerStatistic() :
                PlayFabBaseModel(),
                Id(),
                Name(),
                StatisticValue(),
                StatisticVersion()
            {}

            PlayerStatistic(const PlayerStatistic& src) :
                PlayFabBaseModel(),
                Id(src.Id),
                Name(src.Name),
                StatisticValue(src.StatisticValue),
                StatisticVersion(src.StatisticVersion)
            {}

            ~PlayerStatistic() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["Id"], Id);
                FromJsonUtilS(input["Name"], Name);
                FromJsonUtilP(input["StatisticValue"], StatisticValue);
                FromJsonUtilP(input["StatisticVersion"], StatisticVersion);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_Id; ToJsonUtilS(Id, each_Id); output["Id"] = each_Id;
                Json::Value each_Name; ToJsonUtilS(Name, each_Name); output["Name"] = each_Name;
                Json::Value each_StatisticValue; ToJsonUtilP(StatisticValue, each_StatisticValue); output["StatisticValue"] = each_StatisticValue;
                Json::Value each_StatisticVersion; ToJsonUtilP(StatisticVersion, each_StatisticVersion); output["StatisticVersion"] = each_StatisticVersion;
                return output;
            }
        };

        struct PushNotificationRegistration : public PlayFabBaseModel
        {
            std::string NotificationEndpointARN;
            Boxed<PushNotificationPlatform> Platform;

            PushNotificationRegistration() :
                PlayFabBaseModel(),
                NotificationEndpointARN(),
                Platform()
            {}

            PushNotificationRegistration(const PushNotificationRegistration& src) :
                PlayFabBaseModel(),
                NotificationEndpointARN(src.NotificationEndpointARN),
                Platform(src.Platform)
            {}

            ~PushNotificationRegistration() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["NotificationEndpointARN"], NotificationEndpointARN);
                FromJsonUtilE(input["Platform"], Platform);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_NotificationEndpointARN; ToJsonUtilS(NotificationEndpointARN, each_NotificationEndpointARN); output["NotificationEndpointARN"] = each_NotificationEndpointARN;
                Json::Value each_Platform; ToJsonUtilE(Platform, each_Platform); output["Platform"] = each_Platform;
                return output;
            }
        };

        struct PlayerProfile : public PlayFabBaseModel
        {
            std::list<AdCampaignAttribution> AdCampaignAttributions;
            std::string AvatarUrl;
            Boxed<time_t> BannedUntil;
            std::list<ContactEmailInfo> ContactEmailAddresses;
            Boxed<time_t> Created;
            std::string DisplayName;
            Boxed<time_t> LastLogin;
            std::list<PlayerLinkedAccount> LinkedAccounts;
            std::map<std::string, PlayerLocation> Locations;
            Boxed<LoginIdentityProvider> Origination;
            std::list<std::string> PlayerExperimentVariants;
            std::string PlayerId;
            std::list<PlayerStatistic> PlayerStatistics;
            std::string PublisherId;
            std::list<PushNotificationRegistration> PushNotificationRegistrations;
            std::map<std::string, Int32> Statistics;
            std::list<std::string> Tags;
            std::string TitleId;
            Boxed<Uint32> TotalValueToDateInUSD;
            std::map<std::string, Uint32> ValuesToDate;
            std::map<std::string, Int32> VirtualCurrencyBalances;

            PlayerProfile() :
                PlayFabBaseModel(),
                AdCampaignAttributions(),
                AvatarUrl(),
                BannedUntil(),
                ContactEmailAddresses(),
                Created(),
                DisplayName(),
                LastLogin(),
                LinkedAccounts(),
                Locations(),
                Origination(),
                PlayerExperimentVariants(),
                PlayerId(),
                PlayerStatistics(),
                PublisherId(),
                PushNotificationRegistrations(),
                Statistics(),
                Tags(),
                TitleId(),
                TotalValueToDateInUSD(),
                ValuesToDate(),
                VirtualCurrencyBalances()
            {}

            PlayerProfile(const PlayerProfile& src) :
                PlayFabBaseModel(),
                AdCampaignAttributions(src.AdCampaignAttributions),
                AvatarUrl(src.AvatarUrl),
                BannedUntil(src.BannedUntil),
                ContactEmailAddresses(src.ContactEmailAddresses),
                Created(src.Created),
                DisplayName(src.DisplayName),
                LastLogin(src.LastLogin),
                LinkedAccounts(src.LinkedAccounts),
                Locations(src.Locations),
                Origination(src.Origination),
                PlayerExperimentVariants(src.PlayerExperimentVariants),
                PlayerId(src.PlayerId),
                PlayerStatistics(src.PlayerStatistics),
                PublisherId(src.PublisherId),
                PushNotificationRegistrations(src.PushNotificationRegistrations),
                Statistics(src.Statistics),
                Tags(src.Tags),
                TitleId(src.TitleId),
                TotalValueToDateInUSD(src.TotalValueToDateInUSD),
                ValuesToDate(src.ValuesToDate),
                VirtualCurrencyBalances(src.VirtualCurrencyBalances)
            {}

            ~PlayerProfile() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilO(input["AdCampaignAttributions"], AdCampaignAttributions);
                FromJsonUtilS(input["AvatarUrl"], AvatarUrl);
                FromJsonUtilT(input["BannedUntil"], BannedUntil);
                FromJsonUtilO(input["ContactEmailAddresses"], ContactEmailAddresses);
                FromJsonUtilT(input["Created"], Created);
                FromJsonUtilS(input["DisplayName"], DisplayName);
                FromJsonUtilT(input["LastLogin"], LastLogin);
                FromJsonUtilO(input["LinkedAccounts"], LinkedAccounts);
                FromJsonUtilO(input["Locations"], Locations);
                FromJsonUtilE(input["Origination"], Origination);
                FromJsonUtilS(input["PlayerExperimentVariants"], PlayerExperimentVariants);
                FromJsonUtilS(input["PlayerId"], PlayerId);
                FromJsonUtilO(input["PlayerStatistics"], PlayerStatistics);
                FromJsonUtilS(input["PublisherId"], PublisherId);
                FromJsonUtilO(input["PushNotificationRegistrations"], PushNotificationRegistrations);
                FromJsonUtilP(input["Statistics"], Statistics);
                FromJsonUtilS(input["Tags"], Tags);
                FromJsonUtilS(input["TitleId"], TitleId);
                FromJsonUtilP(input["TotalValueToDateInUSD"], TotalValueToDateInUSD);
                FromJsonUtilP(input["ValuesToDate"], ValuesToDate);
                FromJsonUtilP(input["VirtualCurrencyBalances"], VirtualCurrencyBalances);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_AdCampaignAttributions; ToJsonUtilO(AdCampaignAttributions, each_AdCampaignAttributions); output["AdCampaignAttributions"] = each_AdCampaignAttributions;
                Json::Value each_AvatarUrl; ToJsonUtilS(AvatarUrl, each_AvatarUrl); output["AvatarUrl"] = each_AvatarUrl;
                Json::Value each_BannedUntil; ToJsonUtilT(BannedUntil, each_BannedUntil); output["BannedUntil"] = each_BannedUntil;
                Json::Value each_ContactEmailAddresses; ToJsonUtilO(ContactEmailAddresses, each_ContactEmailAddresses); output["ContactEmailAddresses"] = each_ContactEmailAddresses;
                Json::Value each_Created; ToJsonUtilT(Created, each_Created); output["Created"] = each_Created;
                Json::Value each_DisplayName; ToJsonUtilS(DisplayName, each_DisplayName); output["DisplayName"] = each_DisplayName;
                Json::Value each_LastLogin; ToJsonUtilT(LastLogin, each_LastLogin); output["LastLogin"] = each_LastLogin;
                Json::Value each_LinkedAccounts; ToJsonUtilO(LinkedAccounts, each_LinkedAccounts); output["LinkedAccounts"] = each_LinkedAccounts;
                Json::Value each_Locations; ToJsonUtilO(Locations, each_Locations); output["Locations"] = each_Locations;
                Json::Value each_Origination; ToJsonUtilE(Origination, each_Origination); output["Origination"] = each_Origination;
                Json::Value each_PlayerExperimentVariants; ToJsonUtilS(PlayerExperimentVariants, each_PlayerExperimentVariants); output["PlayerExperimentVariants"] = each_PlayerExperimentVariants;
                Json::Value each_PlayerId; ToJsonUtilS(PlayerId, each_PlayerId); output["PlayerId"] = each_PlayerId;
                Json::Value each_PlayerStatistics; ToJsonUtilO(PlayerStatistics, each_PlayerStatistics); output["PlayerStatistics"] = each_PlayerStatistics;
                Json::Value each_PublisherId; ToJsonUtilS(PublisherId, each_PublisherId); output["PublisherId"] = each_PublisherId;
                Json::Value each_PushNotificationRegistrations; ToJsonUtilO(PushNotificationRegistrations, each_PushNotificationRegistrations); output["PushNotificationRegistrations"] = each_PushNotificationRegistrations;
                Json::Value each_Statistics; ToJsonUtilP(Statistics, each_Statistics); output["Statistics"] = each_Statistics;
                Json::Value each_Tags; ToJsonUtilS(Tags, each_Tags); output["Tags"] = each_Tags;
                Json::Value each_TitleId; ToJsonUtilS(TitleId, each_TitleId); output["TitleId"] = each_TitleId;
                Json::Value each_TotalValueToDateInUSD; ToJsonUtilP(TotalValueToDateInUSD, each_TotalValueToDateInUSD); output["TotalValueToDateInUSD"] = each_TotalValueToDateInUSD;
                Json::Value each_ValuesToDate; ToJsonUtilP(ValuesToDate, each_ValuesToDate); output["ValuesToDate"] = each_ValuesToDate;
                Json::Value each_VirtualCurrencyBalances; ToJsonUtilP(VirtualCurrencyBalances, each_VirtualCurrencyBalances); output["VirtualCurrencyBalances"] = each_VirtualCurrencyBalances;
                return output;
            }
        };

        struct GetPlayersInSegmentResult : public PlayFabResultCommon
        {
            std::string ContinuationToken;
            std::list<PlayerProfile> PlayerProfiles;
            Int32 ProfilesInSegment;

            GetPlayersInSegmentResult() :
                PlayFabResultCommon(),
                ContinuationToken(),
                PlayerProfiles(),
                ProfilesInSegment()
            {}

            GetPlayersInSegmentResult(const GetPlayersInSegmentResult& src) :
                PlayFabResultCommon(),
                ContinuationToken(src.ContinuationToken),
                PlayerProfiles(src.PlayerProfiles),
                ProfilesInSegment(src.ProfilesInSegment)
            {}

            ~GetPlayersInSegmentResult() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["ContinuationToken"], ContinuationToken);
                FromJsonUtilO(input["PlayerProfiles"], PlayerProfiles);
                FromJsonUtilP(input["ProfilesInSegment"], ProfilesInSegment);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_ContinuationToken; ToJsonUtilS(ContinuationToken, each_ContinuationToken); output["ContinuationToken"] = each_ContinuationToken;
                Json::Value each_PlayerProfiles; ToJsonUtilO(PlayerProfiles, each_PlayerProfiles); output["PlayerProfiles"] = each_PlayerProfiles;
                Json::Value each_ProfilesInSegment; ToJsonUtilP(ProfilesInSegment, each_ProfilesInSegment); output["ProfilesInSegment"] = each_ProfilesInSegment;
                return output;
            }
        };

        struct GetPlayersSegmentsRequest : public PlayFabRequestCommon
        {
            std::map<std::string, std::string> CustomTags;
            std::string PlayFabId;

            GetPlayersSegmentsRequest() :
                PlayFabRequestCommon(),
                CustomTags(),
                PlayFabId()
            {}

            GetPlayersSegmentsRequest(const GetPlayersSegmentsRequest& src) :
                PlayFabRequestCommon(),
                CustomTags(src.CustomTags),
                PlayFabId(src.PlayFabId)
            {}

            ~GetPlayersSegmentsRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["CustomTags"], CustomTags);
                FromJsonUtilS(input["PlayFabId"], PlayFabId);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_CustomTags; ToJsonUtilS(CustomTags, each_CustomTags); output["CustomTags"] = each_CustomTags;
                Json::Value each_PlayFabId; ToJsonUtilS(PlayFabId, each_PlayFabId); output["PlayFabId"] = each_PlayFabId;
                return output;
            }
        };

        struct GetPlayerStatisticDefinitionsRequest : public PlayFabRequestCommon
        {

            GetPlayerStatisticDefinitionsRequest() :
                PlayFabRequestCommon()
            {}

            GetPlayerStatisticDefinitionsRequest(const GetPlayerStatisticDefinitionsRequest&) :
                PlayFabRequestCommon()
            {}

            ~GetPlayerStatisticDefinitionsRequest() = default;

            void FromJson(const Json::Value&) override
            {
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                return output;
            }
        };

        struct GetPlayerStatisticDefinitionsResult : public PlayFabResultCommon
        {
            std::list<PlayerStatisticDefinition> Statistics;

            GetPlayerStatisticDefinitionsResult() :
                PlayFabResultCommon(),
                Statistics()
            {}

            GetPlayerStatisticDefinitionsResult(const GetPlayerStatisticDefinitionsResult& src) :
                PlayFabResultCommon(),
                Statistics(src.Statistics)
            {}

            ~GetPlayerStatisticDefinitionsResult() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilO(input["Statistics"], Statistics);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_Statistics; ToJsonUtilO(Statistics, each_Statistics); output["Statistics"] = each_Statistics;
                return output;
            }
        };

        struct GetPlayerStatisticVersionsRequest : public PlayFabRequestCommon
        {
            std::map<std::string, std::string> CustomTags;
            std::string StatisticName;

            GetPlayerStatisticVersionsRequest() :
                PlayFabRequestCommon(),
                CustomTags(),
                StatisticName()
            {}

            GetPlayerStatisticVersionsRequest(const GetPlayerStatisticVersionsRequest& src) :
                PlayFabRequestCommon(),
                CustomTags(src.CustomTags),
                StatisticName(src.StatisticName)
            {}

            ~GetPlayerStatisticVersionsRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["CustomTags"], CustomTags);
                FromJsonUtilS(input["StatisticName"], StatisticName);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_CustomTags; ToJsonUtilS(CustomTags, each_CustomTags); output["CustomTags"] = each_CustomTags;
                Json::Value each_StatisticName; ToJsonUtilS(StatisticName, each_StatisticName); output["StatisticName"] = each_StatisticName;
                return output;
            }
        };

        struct PlayerStatisticVersion : public PlayFabBaseModel
        {
            time_t ActivationTime;
            std::string ArchiveDownloadUrl;
            Boxed<time_t> DeactivationTime;
            Boxed<time_t> ScheduledActivationTime;
            Boxed<time_t> ScheduledDeactivationTime;
            std::string StatisticName;
            Boxed<StatisticVersionStatus> Status;
            Uint32 Version;

            PlayerStatisticVersion() :
                PlayFabBaseModel(),
                ActivationTime(),
                ArchiveDownloadUrl(),
                DeactivationTime(),
                ScheduledActivationTime(),
                ScheduledDeactivationTime(),
                StatisticName(),
                Status(),
                Version()
            {}

            PlayerStatisticVersion(const PlayerStatisticVersion& src) :
                PlayFabBaseModel(),
                ActivationTime(src.ActivationTime),
                ArchiveDownloadUrl(src.ArchiveDownloadUrl),
                DeactivationTime(src.DeactivationTime),
                ScheduledActivationTime(src.ScheduledActivationTime),
                ScheduledDeactivationTime(src.ScheduledDeactivationTime),
                StatisticName(src.StatisticName),
                Status(src.Status),
                Version(src.Version)
            {}

            ~PlayerStatisticVersion() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilT(input["ActivationTime"], ActivationTime);
                FromJsonUtilS(input["ArchiveDownloadUrl"], ArchiveDownloadUrl);
                FromJsonUtilT(input["DeactivationTime"], DeactivationTime);
                FromJsonUtilT(input["ScheduledActivationTime"], ScheduledActivationTime);
                FromJsonUtilT(input["ScheduledDeactivationTime"], ScheduledDeactivationTime);
                FromJsonUtilS(input["StatisticName"], StatisticName);
                FromJsonUtilE(input["Status"], Status);
                FromJsonUtilP(input["Version"], Version);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_ActivationTime; ToJsonUtilT(ActivationTime, each_ActivationTime); output["ActivationTime"] = each_ActivationTime;
                Json::Value each_ArchiveDownloadUrl; ToJsonUtilS(ArchiveDownloadUrl, each_ArchiveDownloadUrl); output["ArchiveDownloadUrl"] = each_ArchiveDownloadUrl;
                Json::Value each_DeactivationTime; ToJsonUtilT(DeactivationTime, each_DeactivationTime); output["DeactivationTime"] = each_DeactivationTime;
                Json::Value each_ScheduledActivationTime; ToJsonUtilT(ScheduledActivationTime, each_ScheduledActivationTime); output["ScheduledActivationTime"] = each_ScheduledActivationTime;
                Json::Value each_ScheduledDeactivationTime; ToJsonUtilT(ScheduledDeactivationTime, each_ScheduledDeactivationTime); output["ScheduledDeactivationTime"] = each_ScheduledDeactivationTime;
                Json::Value each_StatisticName; ToJsonUtilS(StatisticName, each_StatisticName); output["StatisticName"] = each_StatisticName;
                Json::Value each_Status; ToJsonUtilE(Status, each_Status); output["Status"] = each_Status;
                Json::Value each_Version; ToJsonUtilP(Version, each_Version); output["Version"] = each_Version;
                return output;
            }
        };

        struct GetPlayerStatisticVersionsResult : public PlayFabResultCommon
        {
            std::list<PlayerStatisticVersion> StatisticVersions;

            GetPlayerStatisticVersionsResult() :
                PlayFabResultCommon(),
                StatisticVersions()
            {}

            GetPlayerStatisticVersionsResult(const GetPlayerStatisticVersionsResult& src) :
                PlayFabResultCommon(),
                StatisticVersions(src.StatisticVersions)
            {}

            ~GetPlayerStatisticVersionsResult() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilO(input["StatisticVersions"], StatisticVersions);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_StatisticVersions; ToJsonUtilO(StatisticVersions, each_StatisticVersions); output["StatisticVersions"] = each_StatisticVersions;
                return output;
            }
        };

        struct GetPlayerTagsRequest : public PlayFabRequestCommon
        {
            std::map<std::string, std::string> CustomTags;
            std::string Namespace;
            std::string PlayFabId;

            GetPlayerTagsRequest() :
                PlayFabRequestCommon(),
                CustomTags(),
                Namespace(),
                PlayFabId()
            {}

            GetPlayerTagsRequest(const GetPlayerTagsRequest& src) :
                PlayFabRequestCommon(),
                CustomTags(src.CustomTags),
                Namespace(src.Namespace),
                PlayFabId(src.PlayFabId)
            {}

            ~GetPlayerTagsRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["CustomTags"], CustomTags);
                FromJsonUtilS(input["Namespace"], Namespace);
                FromJsonUtilS(input["PlayFabId"], PlayFabId);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_CustomTags; ToJsonUtilS(CustomTags, each_CustomTags); output["CustomTags"] = each_CustomTags;
                Json::Value each_Namespace; ToJsonUtilS(Namespace, each_Namespace); output["Namespace"] = each_Namespace;
                Json::Value each_PlayFabId; ToJsonUtilS(PlayFabId, each_PlayFabId); output["PlayFabId"] = each_PlayFabId;
                return output;
            }
        };

        struct GetPlayerTagsResult : public PlayFabResultCommon
        {
            std::string PlayFabId;
            std::list<std::string> Tags;

            GetPlayerTagsResult() :
                PlayFabResultCommon(),
                PlayFabId(),
                Tags()
            {}

            GetPlayerTagsResult(const GetPlayerTagsResult& src) :
                PlayFabResultCommon(),
                PlayFabId(src.PlayFabId),
                Tags(src.Tags)
            {}

            ~GetPlayerTagsResult() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["PlayFabId"], PlayFabId);
                FromJsonUtilS(input["Tags"], Tags);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_PlayFabId; ToJsonUtilS(PlayFabId, each_PlayFabId); output["PlayFabId"] = each_PlayFabId;
                Json::Value each_Tags; ToJsonUtilS(Tags, each_Tags); output["Tags"] = each_Tags;
                return output;
            }
        };

        struct GetPolicyRequest : public PlayFabRequestCommon
        {
            std::string PolicyName;

            GetPolicyRequest() :
                PlayFabRequestCommon(),
                PolicyName()
            {}

            GetPolicyRequest(const GetPolicyRequest& src) :
                PlayFabRequestCommon(),
                PolicyName(src.PolicyName)
            {}

            ~GetPolicyRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["PolicyName"], PolicyName);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_PolicyName; ToJsonUtilS(PolicyName, each_PolicyName); output["PolicyName"] = each_PolicyName;
                return output;
            }
        };

        struct PermissionStatement : public PlayFabBaseModel
        {
            std::string Action;
            Boxed<ApiCondition> ApiConditions;
            std::string Comment;
            EffectType Effect;
            std::string Principal;
            std::string Resource;

            PermissionStatement() :
                PlayFabBaseModel(),
                Action(),
                ApiConditions(),
                Comment(),
                Effect(),
                Principal(),
                Resource()
            {}

            PermissionStatement(const PermissionStatement& src) :
                PlayFabBaseModel(),
                Action(src.Action),
                ApiConditions(src.ApiConditions),
                Comment(src.Comment),
                Effect(src.Effect),
                Principal(src.Principal),
                Resource(src.Resource)
            {}

            ~PermissionStatement() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["Action"], Action);
                FromJsonUtilO(input["ApiConditions"], ApiConditions);
                FromJsonUtilS(input["Comment"], Comment);
                FromJsonEnum(input["Effect"], Effect);
                FromJsonUtilS(input["Principal"], Principal);
                FromJsonUtilS(input["Resource"], Resource);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_Action; ToJsonUtilS(Action, each_Action); output["Action"] = each_Action;
                Json::Value each_ApiConditions; ToJsonUtilO(ApiConditions, each_ApiConditions); output["ApiConditions"] = each_ApiConditions;
                Json::Value each_Comment; ToJsonUtilS(Comment, each_Comment); output["Comment"] = each_Comment;
                Json::Value each_Effect; ToJsonEnum(Effect, each_Effect); output["Effect"] = each_Effect;
                Json::Value each_Principal; ToJsonUtilS(Principal, each_Principal); output["Principal"] = each_Principal;
                Json::Value each_Resource; ToJsonUtilS(Resource, each_Resource); output["Resource"] = each_Resource;
                return output;
            }
        };

        struct GetPolicyResponse : public PlayFabResultCommon
        {
            std::string PolicyName;
            std::list<PermissionStatement> Statements;

            GetPolicyResponse() :
                PlayFabResultCommon(),
                PolicyName(),
                Statements()
            {}

            GetPolicyResponse(const GetPolicyResponse& src) :
                PlayFabResultCommon(),
                PolicyName(src.PolicyName),
                Statements(src.Statements)
            {}

            ~GetPolicyResponse() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["PolicyName"], PolicyName);
                FromJsonUtilO(input["Statements"], Statements);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_PolicyName; ToJsonUtilS(PolicyName, each_PolicyName); output["PolicyName"] = each_PolicyName;
                Json::Value each_Statements; ToJsonUtilO(Statements, each_Statements); output["Statements"] = each_Statements;
                return output;
            }
        };

        struct GetPublisherDataRequest : public PlayFabRequestCommon
        {
            std::list<std::string> Keys;

            GetPublisherDataRequest() :
                PlayFabRequestCommon(),
                Keys()
            {}

            GetPublisherDataRequest(const GetPublisherDataRequest& src) :
                PlayFabRequestCommon(),
                Keys(src.Keys)
            {}

            ~GetPublisherDataRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["Keys"], Keys);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_Keys; ToJsonUtilS(Keys, each_Keys); output["Keys"] = each_Keys;
                return output;
            }
        };

        struct GetPublisherDataResult : public PlayFabResultCommon
        {
            std::map<std::string, std::string> Data;

            GetPublisherDataResult() :
                PlayFabResultCommon(),
                Data()
            {}

            GetPublisherDataResult(const GetPublisherDataResult& src) :
                PlayFabResultCommon(),
                Data(src.Data)
            {}

            ~GetPublisherDataResult() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["Data"], Data);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_Data; ToJsonUtilS(Data, each_Data); output["Data"] = each_Data;
                return output;
            }
        };

        struct GetRandomResultTablesRequest : public PlayFabRequestCommon
        {
            std::string CatalogVersion;

            GetRandomResultTablesRequest() :
                PlayFabRequestCommon(),
                CatalogVersion()
            {}

            GetRandomResultTablesRequest(const GetRandomResultTablesRequest& src) :
                PlayFabRequestCommon(),
                CatalogVersion(src.CatalogVersion)
            {}

            ~GetRandomResultTablesRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["CatalogVersion"], CatalogVersion);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_CatalogVersion; ToJsonUtilS(CatalogVersion, each_CatalogVersion); output["CatalogVersion"] = each_CatalogVersion;
                return output;
            }
        };

        struct ResultTableNode : public PlayFabBaseModel
        {
            std::string ResultItem;
            ResultTableNodeType ResultItemType;
            Int32 Weight;

            ResultTableNode() :
                PlayFabBaseModel(),
                ResultItem(),
                ResultItemType(),
                Weight()
            {}

            ResultTableNode(const ResultTableNode& src) :
                PlayFabBaseModel(),
                ResultItem(src.ResultItem),
                ResultItemType(src.ResultItemType),
                Weight(src.Weight)
            {}

            ~ResultTableNode() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["ResultItem"], ResultItem);
                FromJsonEnum(input["ResultItemType"], ResultItemType);
                FromJsonUtilP(input["Weight"], Weight);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_ResultItem; ToJsonUtilS(ResultItem, each_ResultItem); output["ResultItem"] = each_ResultItem;
                Json::Value each_ResultItemType; ToJsonEnum(ResultItemType, each_ResultItemType); output["ResultItemType"] = each_ResultItemType;
                Json::Value each_Weight; ToJsonUtilP(Weight, each_Weight); output["Weight"] = each_Weight;
                return output;
            }
        };

        struct RandomResultTableListing : public PlayFabBaseModel
        {
            std::string CatalogVersion;
            std::list<ResultTableNode> Nodes;
            std::string TableId;

            RandomResultTableListing() :
                PlayFabBaseModel(),
                CatalogVersion(),
                Nodes(),
                TableId()
            {}

            RandomResultTableListing(const RandomResultTableListing& src) :
                PlayFabBaseModel(),
                CatalogVersion(src.CatalogVersion),
                Nodes(src.Nodes),
                TableId(src.TableId)
            {}

            ~RandomResultTableListing() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["CatalogVersion"], CatalogVersion);
                FromJsonUtilO(input["Nodes"], Nodes);
                FromJsonUtilS(input["TableId"], TableId);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_CatalogVersion; ToJsonUtilS(CatalogVersion, each_CatalogVersion); output["CatalogVersion"] = each_CatalogVersion;
                Json::Value each_Nodes; ToJsonUtilO(Nodes, each_Nodes); output["Nodes"] = each_Nodes;
                Json::Value each_TableId; ToJsonUtilS(TableId, each_TableId); output["TableId"] = each_TableId;
                return output;
            }
        };

        struct GetRandomResultTablesResult : public PlayFabResultCommon
        {
            std::map<std::string, RandomResultTableListing> Tables;

            GetRandomResultTablesResult() :
                PlayFabResultCommon(),
                Tables()
            {}

            GetRandomResultTablesResult(const GetRandomResultTablesResult& src) :
                PlayFabResultCommon(),
                Tables(src.Tables)
            {}

            ~GetRandomResultTablesResult() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilO(input["Tables"], Tables);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_Tables; ToJsonUtilO(Tables, each_Tables); output["Tables"] = each_Tables;
                return output;
            }
        };

        struct GetServerBuildInfoRequest : public PlayFabRequestCommon
        {
            std::string BuildId;

            GetServerBuildInfoRequest() :
                PlayFabRequestCommon(),
                BuildId()
            {}

            GetServerBuildInfoRequest(const GetServerBuildInfoRequest& src) :
                PlayFabRequestCommon(),
                BuildId(src.BuildId)
            {}

            ~GetServerBuildInfoRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["BuildId"], BuildId);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_BuildId; ToJsonUtilS(BuildId, each_BuildId); output["BuildId"] = each_BuildId;
                return output;
            }
        };

        struct GetServerBuildInfoResult : public PlayFabResultCommon
        {
            std::list<Region> ActiveRegions;
            std::string BuildId;
            std::string Comment;
            std::string ErrorMessage;
            Int32 MaxGamesPerHost;
            Int32 MinFreeGameSlots;
            Boxed<GameBuildStatus> Status;
            time_t Timestamp;
            std::string TitleId;

            GetServerBuildInfoResult() :
                PlayFabResultCommon(),
                ActiveRegions(),
                BuildId(),
                Comment(),
                ErrorMessage(),
                MaxGamesPerHost(),
                MinFreeGameSlots(),
                Status(),
                Timestamp(),
                TitleId()
            {}

            GetServerBuildInfoResult(const GetServerBuildInfoResult& src) :
                PlayFabResultCommon(),
                ActiveRegions(src.ActiveRegions),
                BuildId(src.BuildId),
                Comment(src.Comment),
                ErrorMessage(src.ErrorMessage),
                MaxGamesPerHost(src.MaxGamesPerHost),
                MinFreeGameSlots(src.MinFreeGameSlots),
                Status(src.Status),
                Timestamp(src.Timestamp),
                TitleId(src.TitleId)
            {}

            ~GetServerBuildInfoResult() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilE(input["ActiveRegions"], ActiveRegions);
                FromJsonUtilS(input["BuildId"], BuildId);
                FromJsonUtilS(input["Comment"], Comment);
                FromJsonUtilS(input["ErrorMessage"], ErrorMessage);
                FromJsonUtilP(input["MaxGamesPerHost"], MaxGamesPerHost);
                FromJsonUtilP(input["MinFreeGameSlots"], MinFreeGameSlots);
                FromJsonUtilE(input["Status"], Status);
                FromJsonUtilT(input["Timestamp"], Timestamp);
                FromJsonUtilS(input["TitleId"], TitleId);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_ActiveRegions; ToJsonUtilE(ActiveRegions, each_ActiveRegions); output["ActiveRegions"] = each_ActiveRegions;
                Json::Value each_BuildId; ToJsonUtilS(BuildId, each_BuildId); output["BuildId"] = each_BuildId;
                Json::Value each_Comment; ToJsonUtilS(Comment, each_Comment); output["Comment"] = each_Comment;
                Json::Value each_ErrorMessage; ToJsonUtilS(ErrorMessage, each_ErrorMessage); output["ErrorMessage"] = each_ErrorMessage;
                Json::Value each_MaxGamesPerHost; ToJsonUtilP(MaxGamesPerHost, each_MaxGamesPerHost); output["MaxGamesPerHost"] = each_MaxGamesPerHost;
                Json::Value each_MinFreeGameSlots; ToJsonUtilP(MinFreeGameSlots, each_MinFreeGameSlots); output["MinFreeGameSlots"] = each_MinFreeGameSlots;
                Json::Value each_Status; ToJsonUtilE(Status, each_Status); output["Status"] = each_Status;
                Json::Value each_Timestamp; ToJsonUtilT(Timestamp, each_Timestamp); output["Timestamp"] = each_Timestamp;
                Json::Value each_TitleId; ToJsonUtilS(TitleId, each_TitleId); output["TitleId"] = each_TitleId;
                return output;
            }
        };

        struct GetServerBuildUploadURLRequest : public PlayFabRequestCommon
        {
            std::string BuildId;

            GetServerBuildUploadURLRequest() :
                PlayFabRequestCommon(),
                BuildId()
            {}

            GetServerBuildUploadURLRequest(const GetServerBuildUploadURLRequest& src) :
                PlayFabRequestCommon(),
                BuildId(src.BuildId)
            {}

            ~GetServerBuildUploadURLRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["BuildId"], BuildId);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_BuildId; ToJsonUtilS(BuildId, each_BuildId); output["BuildId"] = each_BuildId;
                return output;
            }
        };

        struct GetServerBuildUploadURLResult : public PlayFabResultCommon
        {
            std::string URL;

            GetServerBuildUploadURLResult() :
                PlayFabResultCommon(),
                URL()
            {}

            GetServerBuildUploadURLResult(const GetServerBuildUploadURLResult& src) :
                PlayFabResultCommon(),
                URL(src.URL)
            {}

            ~GetServerBuildUploadURLResult() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["URL"], URL);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_URL; ToJsonUtilS(URL, each_URL); output["URL"] = each_URL;
                return output;
            }
        };

        struct GetStoreItemsRequest : public PlayFabRequestCommon
        {
            std::string CatalogVersion;
            std::string StoreId;

            GetStoreItemsRequest() :
                PlayFabRequestCommon(),
                CatalogVersion(),
                StoreId()
            {}

            GetStoreItemsRequest(const GetStoreItemsRequest& src) :
                PlayFabRequestCommon(),
                CatalogVersion(src.CatalogVersion),
                StoreId(src.StoreId)
            {}

            ~GetStoreItemsRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["CatalogVersion"], CatalogVersion);
                FromJsonUtilS(input["StoreId"], StoreId);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_CatalogVersion; ToJsonUtilS(CatalogVersion, each_CatalogVersion); output["CatalogVersion"] = each_CatalogVersion;
                Json::Value each_StoreId; ToJsonUtilS(StoreId, each_StoreId); output["StoreId"] = each_StoreId;
                return output;
            }
        };

        struct StoreMarketingModel : public PlayFabBaseModel
        {
            std::string Description;
            std::string DisplayName;
            Json::Value Metadata;

            StoreMarketingModel() :
                PlayFabBaseModel(),
                Description(),
                DisplayName(),
                Metadata()
            {}

            StoreMarketingModel(const StoreMarketingModel& src) :
                PlayFabBaseModel(),
                Description(src.Description),
                DisplayName(src.DisplayName),
                Metadata(src.Metadata)
            {}

            ~StoreMarketingModel() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["Description"], Description);
                FromJsonUtilS(input["DisplayName"], DisplayName);
                Metadata = input["Metadata"];
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_Description; ToJsonUtilS(Description, each_Description); output["Description"] = each_Description;
                Json::Value each_DisplayName; ToJsonUtilS(DisplayName, each_DisplayName); output["DisplayName"] = each_DisplayName;
                output["Metadata"] = Metadata;
                return output;
            }
        };

        struct StoreItem : public PlayFabBaseModel
        {
            Json::Value CustomData;
            Boxed<Uint32> DisplayPosition;
            std::string ItemId;
            std::map<std::string, Uint32> RealCurrencyPrices;
            std::map<std::string, Uint32> VirtualCurrencyPrices;

            StoreItem() :
                PlayFabBaseModel(),
                CustomData(),
                DisplayPosition(),
                ItemId(),
                RealCurrencyPrices(),
                VirtualCurrencyPrices()
            {}

            StoreItem(const StoreItem& src) :
                PlayFabBaseModel(),
                CustomData(src.CustomData),
                DisplayPosition(src.DisplayPosition),
                ItemId(src.ItemId),
                RealCurrencyPrices(src.RealCurrencyPrices),
                VirtualCurrencyPrices(src.VirtualCurrencyPrices)
            {}

            ~StoreItem() = default;

            void FromJson(const Json::Value& input) override
            {
                CustomData = input["CustomData"];
                FromJsonUtilP(input["DisplayPosition"], DisplayPosition);
                FromJsonUtilS(input["ItemId"], ItemId);
                FromJsonUtilP(input["RealCurrencyPrices"], RealCurrencyPrices);
                FromJsonUtilP(input["VirtualCurrencyPrices"], VirtualCurrencyPrices);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                output["CustomData"] = CustomData;
                Json::Value each_DisplayPosition; ToJsonUtilP(DisplayPosition, each_DisplayPosition); output["DisplayPosition"] = each_DisplayPosition;
                Json::Value each_ItemId; ToJsonUtilS(ItemId, each_ItemId); output["ItemId"] = each_ItemId;
                Json::Value each_RealCurrencyPrices; ToJsonUtilP(RealCurrencyPrices, each_RealCurrencyPrices); output["RealCurrencyPrices"] = each_RealCurrencyPrices;
                Json::Value each_VirtualCurrencyPrices; ToJsonUtilP(VirtualCurrencyPrices, each_VirtualCurrencyPrices); output["VirtualCurrencyPrices"] = each_VirtualCurrencyPrices;
                return output;
            }
        };

        struct GetStoreItemsResult : public PlayFabResultCommon
        {
            std::string CatalogVersion;
            Boxed<StoreMarketingModel> MarketingData;
            Boxed<SourceType> Source;
            std::list<StoreItem> Store;
            std::string StoreId;

            GetStoreItemsResult() :
                PlayFabResultCommon(),
                CatalogVersion(),
                MarketingData(),
                Source(),
                Store(),
                StoreId()
            {}

            GetStoreItemsResult(const GetStoreItemsResult& src) :
                PlayFabResultCommon(),
                CatalogVersion(src.CatalogVersion),
                MarketingData(src.MarketingData),
                Source(src.Source),
                Store(src.Store),
                StoreId(src.StoreId)
            {}

            ~GetStoreItemsResult() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["CatalogVersion"], CatalogVersion);
                FromJsonUtilO(input["MarketingData"], MarketingData);
                FromJsonUtilE(input["Source"], Source);
                FromJsonUtilO(input["Store"], Store);
                FromJsonUtilS(input["StoreId"], StoreId);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_CatalogVersion; ToJsonUtilS(CatalogVersion, each_CatalogVersion); output["CatalogVersion"] = each_CatalogVersion;
                Json::Value each_MarketingData; ToJsonUtilO(MarketingData, each_MarketingData); output["MarketingData"] = each_MarketingData;
                Json::Value each_Source; ToJsonUtilE(Source, each_Source); output["Source"] = each_Source;
                Json::Value each_Store; ToJsonUtilO(Store, each_Store); output["Store"] = each_Store;
                Json::Value each_StoreId; ToJsonUtilS(StoreId, each_StoreId); output["StoreId"] = each_StoreId;
                return output;
            }
        };

        struct GetTaskInstanceRequest : public PlayFabRequestCommon
        {
            std::string TaskInstanceId;

            GetTaskInstanceRequest() :
                PlayFabRequestCommon(),
                TaskInstanceId()
            {}

            GetTaskInstanceRequest(const GetTaskInstanceRequest& src) :
                PlayFabRequestCommon(),
                TaskInstanceId(src.TaskInstanceId)
            {}

            ~GetTaskInstanceRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["TaskInstanceId"], TaskInstanceId);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_TaskInstanceId; ToJsonUtilS(TaskInstanceId, each_TaskInstanceId); output["TaskInstanceId"] = each_TaskInstanceId;
                return output;
            }
        };

        struct GetTaskInstancesRequest : public PlayFabRequestCommon
        {
            Boxed<time_t> StartedAtRangeFrom;
            Boxed<time_t> StartedAtRangeTo;
            Boxed<TaskInstanceStatus> StatusFilter;
            Boxed<NameIdentifier> TaskIdentifier;

            GetTaskInstancesRequest() :
                PlayFabRequestCommon(),
                StartedAtRangeFrom(),
                StartedAtRangeTo(),
                StatusFilter(),
                TaskIdentifier()
            {}

            GetTaskInstancesRequest(const GetTaskInstancesRequest& src) :
                PlayFabRequestCommon(),
                StartedAtRangeFrom(src.StartedAtRangeFrom),
                StartedAtRangeTo(src.StartedAtRangeTo),
                StatusFilter(src.StatusFilter),
                TaskIdentifier(src.TaskIdentifier)
            {}

            ~GetTaskInstancesRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilT(input["StartedAtRangeFrom"], StartedAtRangeFrom);
                FromJsonUtilT(input["StartedAtRangeTo"], StartedAtRangeTo);
                FromJsonUtilE(input["StatusFilter"], StatusFilter);
                FromJsonUtilO(input["TaskIdentifier"], TaskIdentifier);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_StartedAtRangeFrom; ToJsonUtilT(StartedAtRangeFrom, each_StartedAtRangeFrom); output["StartedAtRangeFrom"] = each_StartedAtRangeFrom;
                Json::Value each_StartedAtRangeTo; ToJsonUtilT(StartedAtRangeTo, each_StartedAtRangeTo); output["StartedAtRangeTo"] = each_StartedAtRangeTo;
                Json::Value each_StatusFilter; ToJsonUtilE(StatusFilter, each_StatusFilter); output["StatusFilter"] = each_StatusFilter;
                Json::Value each_TaskIdentifier; ToJsonUtilO(TaskIdentifier, each_TaskIdentifier); output["TaskIdentifier"] = each_TaskIdentifier;
                return output;
            }
        };

        struct TaskInstanceBasicSummary : public PlayFabBaseModel
        {
            Boxed<time_t> CompletedAt;
            std::string ErrorMessage;
            Boxed<double> EstimatedSecondsRemaining;
            Boxed<double> PercentComplete;
            std::string ScheduledByUserId;
            time_t StartedAt;
            Boxed<TaskInstanceStatus> Status;
            Boxed<NameIdentifier> TaskIdentifier;
            std::string TaskInstanceId;
            Boxed<ScheduledTaskType> Type;

            TaskInstanceBasicSummary() :
                PlayFabBaseModel(),
                CompletedAt(),
                ErrorMessage(),
                EstimatedSecondsRemaining(),
                PercentComplete(),
                ScheduledByUserId(),
                StartedAt(),
                Status(),
                TaskIdentifier(),
                TaskInstanceId(),
                Type()
            {}

            TaskInstanceBasicSummary(const TaskInstanceBasicSummary& src) :
                PlayFabBaseModel(),
                CompletedAt(src.CompletedAt),
                ErrorMessage(src.ErrorMessage),
                EstimatedSecondsRemaining(src.EstimatedSecondsRemaining),
                PercentComplete(src.PercentComplete),
                ScheduledByUserId(src.ScheduledByUserId),
                StartedAt(src.StartedAt),
                Status(src.Status),
                TaskIdentifier(src.TaskIdentifier),
                TaskInstanceId(src.TaskInstanceId),
                Type(src.Type)
            {}

            ~TaskInstanceBasicSummary() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilT(input["CompletedAt"], CompletedAt);
                FromJsonUtilS(input["ErrorMessage"], ErrorMessage);
                FromJsonUtilP(input["EstimatedSecondsRemaining"], EstimatedSecondsRemaining);
                FromJsonUtilP(input["PercentComplete"], PercentComplete);
                FromJsonUtilS(input["ScheduledByUserId"], ScheduledByUserId);
                FromJsonUtilT(input["StartedAt"], StartedAt);
                FromJsonUtilE(input["Status"], Status);
                FromJsonUtilO(input["TaskIdentifier"], TaskIdentifier);
                FromJsonUtilS(input["TaskInstanceId"], TaskInstanceId);
                FromJsonUtilE(input["Type"], Type);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_CompletedAt; ToJsonUtilT(CompletedAt, each_CompletedAt); output["CompletedAt"] = each_CompletedAt;
                Json::Value each_ErrorMessage; ToJsonUtilS(ErrorMessage, each_ErrorMessage); output["ErrorMessage"] = each_ErrorMessage;
                Json::Value each_EstimatedSecondsRemaining; ToJsonUtilP(EstimatedSecondsRemaining, each_EstimatedSecondsRemaining); output["EstimatedSecondsRemaining"] = each_EstimatedSecondsRemaining;
                Json::Value each_PercentComplete; ToJsonUtilP(PercentComplete, each_PercentComplete); output["PercentComplete"] = each_PercentComplete;
                Json::Value each_ScheduledByUserId; ToJsonUtilS(ScheduledByUserId, each_ScheduledByUserId); output["ScheduledByUserId"] = each_ScheduledByUserId;
                Json::Value each_StartedAt; ToJsonUtilT(StartedAt, each_StartedAt); output["StartedAt"] = each_StartedAt;
                Json::Value each_Status; ToJsonUtilE(Status, each_Status); output["Status"] = each_Status;
                Json::Value each_TaskIdentifier; ToJsonUtilO(TaskIdentifier, each_TaskIdentifier); output["TaskIdentifier"] = each_TaskIdentifier;
                Json::Value each_TaskInstanceId; ToJsonUtilS(TaskInstanceId, each_TaskInstanceId); output["TaskInstanceId"] = each_TaskInstanceId;
                Json::Value each_Type; ToJsonUtilE(Type, each_Type); output["Type"] = each_Type;
                return output;
            }
        };

        struct GetTaskInstancesResult : public PlayFabResultCommon
        {
            std::list<TaskInstanceBasicSummary> Summaries;

            GetTaskInstancesResult() :
                PlayFabResultCommon(),
                Summaries()
            {}

            GetTaskInstancesResult(const GetTaskInstancesResult& src) :
                PlayFabResultCommon(),
                Summaries(src.Summaries)
            {}

            ~GetTaskInstancesResult() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilO(input["Summaries"], Summaries);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_Summaries; ToJsonUtilO(Summaries, each_Summaries); output["Summaries"] = each_Summaries;
                return output;
            }
        };

        struct GetTasksRequest : public PlayFabRequestCommon
        {
            Boxed<NameIdentifier> Identifier;

            GetTasksRequest() :
                PlayFabRequestCommon(),
                Identifier()
            {}

            GetTasksRequest(const GetTasksRequest& src) :
                PlayFabRequestCommon(),
                Identifier(src.Identifier)
            {}

            ~GetTasksRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilO(input["Identifier"], Identifier);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_Identifier; ToJsonUtilO(Identifier, each_Identifier); output["Identifier"] = each_Identifier;
                return output;
            }
        };

        struct ScheduledTask : public PlayFabBaseModel
        {
            std::string Description;
            bool IsActive;
            Boxed<time_t> LastRunTime;
            std::string Name;
            Boxed<time_t> NextRunTime;
            Json::Value Parameter;
            std::string Schedule;
            std::string TaskId;
            Boxed<ScheduledTaskType> Type;

            ScheduledTask() :
                PlayFabBaseModel(),
                Description(),
                IsActive(),
                LastRunTime(),
                Name(),
                NextRunTime(),
                Parameter(),
                Schedule(),
                TaskId(),
                Type()
            {}

            ScheduledTask(const ScheduledTask& src) :
                PlayFabBaseModel(),
                Description(src.Description),
                IsActive(src.IsActive),
                LastRunTime(src.LastRunTime),
                Name(src.Name),
                NextRunTime(src.NextRunTime),
                Parameter(src.Parameter),
                Schedule(src.Schedule),
                TaskId(src.TaskId),
                Type(src.Type)
            {}

            ~ScheduledTask() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["Description"], Description);
                FromJsonUtilP(input["IsActive"], IsActive);
                FromJsonUtilT(input["LastRunTime"], LastRunTime);
                FromJsonUtilS(input["Name"], Name);
                FromJsonUtilT(input["NextRunTime"], NextRunTime);
                Parameter = input["Parameter"];
                FromJsonUtilS(input["Schedule"], Schedule);
                FromJsonUtilS(input["TaskId"], TaskId);
                FromJsonUtilE(input["Type"], Type);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_Description; ToJsonUtilS(Description, each_Description); output["Description"] = each_Description;
                Json::Value each_IsActive; ToJsonUtilP(IsActive, each_IsActive); output["IsActive"] = each_IsActive;
                Json::Value each_LastRunTime; ToJsonUtilT(LastRunTime, each_LastRunTime); output["LastRunTime"] = each_LastRunTime;
                Json::Value each_Name; ToJsonUtilS(Name, each_Name); output["Name"] = each_Name;
                Json::Value each_NextRunTime; ToJsonUtilT(NextRunTime, each_NextRunTime); output["NextRunTime"] = each_NextRunTime;
                output["Parameter"] = Parameter;
                Json::Value each_Schedule; ToJsonUtilS(Schedule, each_Schedule); output["Schedule"] = each_Schedule;
                Json::Value each_TaskId; ToJsonUtilS(TaskId, each_TaskId); output["TaskId"] = each_TaskId;
                Json::Value each_Type; ToJsonUtilE(Type, each_Type); output["Type"] = each_Type;
                return output;
            }
        };

        struct GetTasksResult : public PlayFabResultCommon
        {
            std::list<ScheduledTask> Tasks;

            GetTasksResult() :
                PlayFabResultCommon(),
                Tasks()
            {}

            GetTasksResult(const GetTasksResult& src) :
                PlayFabResultCommon(),
                Tasks(src.Tasks)
            {}

            ~GetTasksResult() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilO(input["Tasks"], Tasks);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_Tasks; ToJsonUtilO(Tasks, each_Tasks); output["Tasks"] = each_Tasks;
                return output;
            }
        };

        struct GetTitleDataRequest : public PlayFabRequestCommon
        {
            std::list<std::string> Keys;
            std::string OverrideLabel;

            GetTitleDataRequest() :
                PlayFabRequestCommon(),
                Keys(),
                OverrideLabel()
            {}

            GetTitleDataRequest(const GetTitleDataRequest& src) :
                PlayFabRequestCommon(),
                Keys(src.Keys),
                OverrideLabel(src.OverrideLabel)
            {}

            ~GetTitleDataRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["Keys"], Keys);
                FromJsonUtilS(input["OverrideLabel"], OverrideLabel);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_Keys; ToJsonUtilS(Keys, each_Keys); output["Keys"] = each_Keys;
                Json::Value each_OverrideLabel; ToJsonUtilS(OverrideLabel, each_OverrideLabel); output["OverrideLabel"] = each_OverrideLabel;
                return output;
            }
        };

        struct GetTitleDataResult : public PlayFabResultCommon
        {
            std::map<std::string, std::string> Data;

            GetTitleDataResult() :
                PlayFabResultCommon(),
                Data()
            {}

            GetTitleDataResult(const GetTitleDataResult& src) :
                PlayFabResultCommon(),
                Data(src.Data)
            {}

            ~GetTitleDataResult() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["Data"], Data);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_Data; ToJsonUtilS(Data, each_Data); output["Data"] = each_Data;
                return output;
            }
        };

        struct GetUserBansRequest : public PlayFabRequestCommon
        {
            std::string PlayFabId;

            GetUserBansRequest() :
                PlayFabRequestCommon(),
                PlayFabId()
            {}

            GetUserBansRequest(const GetUserBansRequest& src) :
                PlayFabRequestCommon(),
                PlayFabId(src.PlayFabId)
            {}

            ~GetUserBansRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["PlayFabId"], PlayFabId);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_PlayFabId; ToJsonUtilS(PlayFabId, each_PlayFabId); output["PlayFabId"] = each_PlayFabId;
                return output;
            }
        };

        struct GetUserBansResult : public PlayFabResultCommon
        {
            std::list<BanInfo> BanData;

            GetUserBansResult() :
                PlayFabResultCommon(),
                BanData()
            {}

            GetUserBansResult(const GetUserBansResult& src) :
                PlayFabResultCommon(),
                BanData(src.BanData)
            {}

            ~GetUserBansResult() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilO(input["BanData"], BanData);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_BanData; ToJsonUtilO(BanData, each_BanData); output["BanData"] = each_BanData;
                return output;
            }
        };

        struct GetUserDataRequest : public PlayFabRequestCommon
        {
            Boxed<Uint32> IfChangedFromDataVersion;
            std::list<std::string> Keys;
            std::string PlayFabId;

            GetUserDataRequest() :
                PlayFabRequestCommon(),
                IfChangedFromDataVersion(),
                Keys(),
                PlayFabId()
            {}

            GetUserDataRequest(const GetUserDataRequest& src) :
                PlayFabRequestCommon(),
                IfChangedFromDataVersion(src.IfChangedFromDataVersion),
                Keys(src.Keys),
                PlayFabId(src.PlayFabId)
            {}

            ~GetUserDataRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilP(input["IfChangedFromDataVersion"], IfChangedFromDataVersion);
                FromJsonUtilS(input["Keys"], Keys);
                FromJsonUtilS(input["PlayFabId"], PlayFabId);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_IfChangedFromDataVersion; ToJsonUtilP(IfChangedFromDataVersion, each_IfChangedFromDataVersion); output["IfChangedFromDataVersion"] = each_IfChangedFromDataVersion;
                Json::Value each_Keys; ToJsonUtilS(Keys, each_Keys); output["Keys"] = each_Keys;
                Json::Value each_PlayFabId; ToJsonUtilS(PlayFabId, each_PlayFabId); output["PlayFabId"] = each_PlayFabId;
                return output;
            }
        };

        struct UserDataRecord : public PlayFabBaseModel
        {
            time_t LastUpdated;
            Boxed<UserDataPermission> Permission;
            std::string Value;

            UserDataRecord() :
                PlayFabBaseModel(),
                LastUpdated(),
                Permission(),
                Value()
            {}

            UserDataRecord(const UserDataRecord& src) :
                PlayFabBaseModel(),
                LastUpdated(src.LastUpdated),
                Permission(src.Permission),
                Value(src.Value)
            {}

            ~UserDataRecord() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilT(input["LastUpdated"], LastUpdated);
                FromJsonUtilE(input["Permission"], Permission);
                FromJsonUtilS(input["Value"], Value);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_LastUpdated; ToJsonUtilT(LastUpdated, each_LastUpdated); output["LastUpdated"] = each_LastUpdated;
                Json::Value each_Permission; ToJsonUtilE(Permission, each_Permission); output["Permission"] = each_Permission;
                Json::Value each_Value; ToJsonUtilS(Value, each_Value); output["Value"] = each_Value;
                return output;
            }
        };

        struct GetUserDataResult : public PlayFabResultCommon
        {
            std::map<std::string, UserDataRecord> Data;
            Uint32 DataVersion;
            std::string PlayFabId;

            GetUserDataResult() :
                PlayFabResultCommon(),
                Data(),
                DataVersion(),
                PlayFabId()
            {}

            GetUserDataResult(const GetUserDataResult& src) :
                PlayFabResultCommon(),
                Data(src.Data),
                DataVersion(src.DataVersion),
                PlayFabId(src.PlayFabId)
            {}

            ~GetUserDataResult() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilO(input["Data"], Data);
                FromJsonUtilP(input["DataVersion"], DataVersion);
                FromJsonUtilS(input["PlayFabId"], PlayFabId);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_Data; ToJsonUtilO(Data, each_Data); output["Data"] = each_Data;
                Json::Value each_DataVersion; ToJsonUtilP(DataVersion, each_DataVersion); output["DataVersion"] = each_DataVersion;
                Json::Value each_PlayFabId; ToJsonUtilS(PlayFabId, each_PlayFabId); output["PlayFabId"] = each_PlayFabId;
                return output;
            }
        };

        struct GetUserInventoryRequest : public PlayFabRequestCommon
        {
            std::map<std::string, std::string> CustomTags;
            std::string PlayFabId;

            GetUserInventoryRequest() :
                PlayFabRequestCommon(),
                CustomTags(),
                PlayFabId()
            {}

            GetUserInventoryRequest(const GetUserInventoryRequest& src) :
                PlayFabRequestCommon(),
                CustomTags(src.CustomTags),
                PlayFabId(src.PlayFabId)
            {}

            ~GetUserInventoryRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["CustomTags"], CustomTags);
                FromJsonUtilS(input["PlayFabId"], PlayFabId);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_CustomTags; ToJsonUtilS(CustomTags, each_CustomTags); output["CustomTags"] = each_CustomTags;
                Json::Value each_PlayFabId; ToJsonUtilS(PlayFabId, each_PlayFabId); output["PlayFabId"] = each_PlayFabId;
                return output;
            }
        };

        struct ItemInstance : public PlayFabBaseModel
        {
            std::string Annotation;
            std::list<std::string> BundleContents;
            std::string BundleParent;
            std::string CatalogVersion;
            std::map<std::string, std::string> CustomData;
            std::string DisplayName;
            Boxed<time_t> Expiration;
            std::string ItemClass;
            std::string ItemId;
            std::string ItemInstanceId;
            Boxed<time_t> PurchaseDate;
            Boxed<Int32> RemainingUses;
            std::string UnitCurrency;
            Uint32 UnitPrice;
            Boxed<Int32> UsesIncrementedBy;

            ItemInstance() :
                PlayFabBaseModel(),
                Annotation(),
                BundleContents(),
                BundleParent(),
                CatalogVersion(),
                CustomData(),
                DisplayName(),
                Expiration(),
                ItemClass(),
                ItemId(),
                ItemInstanceId(),
                PurchaseDate(),
                RemainingUses(),
                UnitCurrency(),
                UnitPrice(),
                UsesIncrementedBy()
            {}

            ItemInstance(const ItemInstance& src) :
                PlayFabBaseModel(),
                Annotation(src.Annotation),
                BundleContents(src.BundleContents),
                BundleParent(src.BundleParent),
                CatalogVersion(src.CatalogVersion),
                CustomData(src.CustomData),
                DisplayName(src.DisplayName),
                Expiration(src.Expiration),
                ItemClass(src.ItemClass),
                ItemId(src.ItemId),
                ItemInstanceId(src.ItemInstanceId),
                PurchaseDate(src.PurchaseDate),
                RemainingUses(src.RemainingUses),
                UnitCurrency(src.UnitCurrency),
                UnitPrice(src.UnitPrice),
                UsesIncrementedBy(src.UsesIncrementedBy)
            {}

            ~ItemInstance() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["Annotation"], Annotation);
                FromJsonUtilS(input["BundleContents"], BundleContents);
                FromJsonUtilS(input["BundleParent"], BundleParent);
                FromJsonUtilS(input["CatalogVersion"], CatalogVersion);
                FromJsonUtilS(input["CustomData"], CustomData);
                FromJsonUtilS(input["DisplayName"], DisplayName);
                FromJsonUtilT(input["Expiration"], Expiration);
                FromJsonUtilS(input["ItemClass"], ItemClass);
                FromJsonUtilS(input["ItemId"], ItemId);
                FromJsonUtilS(input["ItemInstanceId"], ItemInstanceId);
                FromJsonUtilT(input["PurchaseDate"], PurchaseDate);
                FromJsonUtilP(input["RemainingUses"], RemainingUses);
                FromJsonUtilS(input["UnitCurrency"], UnitCurrency);
                FromJsonUtilP(input["UnitPrice"], UnitPrice);
                FromJsonUtilP(input["UsesIncrementedBy"], UsesIncrementedBy);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_Annotation; ToJsonUtilS(Annotation, each_Annotation); output["Annotation"] = each_Annotation;
                Json::Value each_BundleContents; ToJsonUtilS(BundleContents, each_BundleContents); output["BundleContents"] = each_BundleContents;
                Json::Value each_BundleParent; ToJsonUtilS(BundleParent, each_BundleParent); output["BundleParent"] = each_BundleParent;
                Json::Value each_CatalogVersion; ToJsonUtilS(CatalogVersion, each_CatalogVersion); output["CatalogVersion"] = each_CatalogVersion;
                Json::Value each_CustomData; ToJsonUtilS(CustomData, each_CustomData); output["CustomData"] = each_CustomData;
                Json::Value each_DisplayName; ToJsonUtilS(DisplayName, each_DisplayName); output["DisplayName"] = each_DisplayName;
                Json::Value each_Expiration; ToJsonUtilT(Expiration, each_Expiration); output["Expiration"] = each_Expiration;
                Json::Value each_ItemClass; ToJsonUtilS(ItemClass, each_ItemClass); output["ItemClass"] = each_ItemClass;
                Json::Value each_ItemId; ToJsonUtilS(ItemId, each_ItemId); output["ItemId"] = each_ItemId;
                Json::Value each_ItemInstanceId; ToJsonUtilS(ItemInstanceId, each_ItemInstanceId); output["ItemInstanceId"] = each_ItemInstanceId;
                Json::Value each_PurchaseDate; ToJsonUtilT(PurchaseDate, each_PurchaseDate); output["PurchaseDate"] = each_PurchaseDate;
                Json::Value each_RemainingUses; ToJsonUtilP(RemainingUses, each_RemainingUses); output["RemainingUses"] = each_RemainingUses;
                Json::Value each_UnitCurrency; ToJsonUtilS(UnitCurrency, each_UnitCurrency); output["UnitCurrency"] = each_UnitCurrency;
                Json::Value each_UnitPrice; ToJsonUtilP(UnitPrice, each_UnitPrice); output["UnitPrice"] = each_UnitPrice;
                Json::Value each_UsesIncrementedBy; ToJsonUtilP(UsesIncrementedBy, each_UsesIncrementedBy); output["UsesIncrementedBy"] = each_UsesIncrementedBy;
                return output;
            }
        };

        struct VirtualCurrencyRechargeTime : public PlayFabBaseModel
        {
            Int32 RechargeMax;
            time_t RechargeTime;
            Int32 SecondsToRecharge;

            VirtualCurrencyRechargeTime() :
                PlayFabBaseModel(),
                RechargeMax(),
                RechargeTime(),
                SecondsToRecharge()
            {}

            VirtualCurrencyRechargeTime(const VirtualCurrencyRechargeTime& src) :
                PlayFabBaseModel(),
                RechargeMax(src.RechargeMax),
                RechargeTime(src.RechargeTime),
                SecondsToRecharge(src.SecondsToRecharge)
            {}

            ~VirtualCurrencyRechargeTime() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilP(input["RechargeMax"], RechargeMax);
                FromJsonUtilT(input["RechargeTime"], RechargeTime);
                FromJsonUtilP(input["SecondsToRecharge"], SecondsToRecharge);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_RechargeMax; ToJsonUtilP(RechargeMax, each_RechargeMax); output["RechargeMax"] = each_RechargeMax;
                Json::Value each_RechargeTime; ToJsonUtilT(RechargeTime, each_RechargeTime); output["RechargeTime"] = each_RechargeTime;
                Json::Value each_SecondsToRecharge; ToJsonUtilP(SecondsToRecharge, each_SecondsToRecharge); output["SecondsToRecharge"] = each_SecondsToRecharge;
                return output;
            }
        };

        struct GetUserInventoryResult : public PlayFabResultCommon
        {
            std::list<ItemInstance> Inventory;
            std::string PlayFabId;
            std::map<std::string, Int32> VirtualCurrency;
            std::map<std::string, VirtualCurrencyRechargeTime> VirtualCurrencyRechargeTimes;

            GetUserInventoryResult() :
                PlayFabResultCommon(),
                Inventory(),
                PlayFabId(),
                VirtualCurrency(),
                VirtualCurrencyRechargeTimes()
            {}

            GetUserInventoryResult(const GetUserInventoryResult& src) :
                PlayFabResultCommon(),
                Inventory(src.Inventory),
                PlayFabId(src.PlayFabId),
                VirtualCurrency(src.VirtualCurrency),
                VirtualCurrencyRechargeTimes(src.VirtualCurrencyRechargeTimes)
            {}

            ~GetUserInventoryResult() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilO(input["Inventory"], Inventory);
                FromJsonUtilS(input["PlayFabId"], PlayFabId);
                FromJsonUtilP(input["VirtualCurrency"], VirtualCurrency);
                FromJsonUtilO(input["VirtualCurrencyRechargeTimes"], VirtualCurrencyRechargeTimes);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_Inventory; ToJsonUtilO(Inventory, each_Inventory); output["Inventory"] = each_Inventory;
                Json::Value each_PlayFabId; ToJsonUtilS(PlayFabId, each_PlayFabId); output["PlayFabId"] = each_PlayFabId;
                Json::Value each_VirtualCurrency; ToJsonUtilP(VirtualCurrency, each_VirtualCurrency); output["VirtualCurrency"] = each_VirtualCurrency;
                Json::Value each_VirtualCurrencyRechargeTimes; ToJsonUtilO(VirtualCurrencyRechargeTimes, each_VirtualCurrencyRechargeTimes); output["VirtualCurrencyRechargeTimes"] = each_VirtualCurrencyRechargeTimes;
                return output;
            }
        };

        struct GrantedItemInstance : public PlayFabBaseModel
        {
            std::string Annotation;
            std::list<std::string> BundleContents;
            std::string BundleParent;
            std::string CatalogVersion;
            std::string CharacterId;
            std::map<std::string, std::string> CustomData;
            std::string DisplayName;
            Boxed<time_t> Expiration;
            std::string ItemClass;
            std::string ItemId;
            std::string ItemInstanceId;
            std::string PlayFabId;
            Boxed<time_t> PurchaseDate;
            Boxed<Int32> RemainingUses;
            bool Result;
            std::string UnitCurrency;
            Uint32 UnitPrice;
            Boxed<Int32> UsesIncrementedBy;

            GrantedItemInstance() :
                PlayFabBaseModel(),
                Annotation(),
                BundleContents(),
                BundleParent(),
                CatalogVersion(),
                CharacterId(),
                CustomData(),
                DisplayName(),
                Expiration(),
                ItemClass(),
                ItemId(),
                ItemInstanceId(),
                PlayFabId(),
                PurchaseDate(),
                RemainingUses(),
                Result(),
                UnitCurrency(),
                UnitPrice(),
                UsesIncrementedBy()
            {}

            GrantedItemInstance(const GrantedItemInstance& src) :
                PlayFabBaseModel(),
                Annotation(src.Annotation),
                BundleContents(src.BundleContents),
                BundleParent(src.BundleParent),
                CatalogVersion(src.CatalogVersion),
                CharacterId(src.CharacterId),
                CustomData(src.CustomData),
                DisplayName(src.DisplayName),
                Expiration(src.Expiration),
                ItemClass(src.ItemClass),
                ItemId(src.ItemId),
                ItemInstanceId(src.ItemInstanceId),
                PlayFabId(src.PlayFabId),
                PurchaseDate(src.PurchaseDate),
                RemainingUses(src.RemainingUses),
                Result(src.Result),
                UnitCurrency(src.UnitCurrency),
                UnitPrice(src.UnitPrice),
                UsesIncrementedBy(src.UsesIncrementedBy)
            {}

            ~GrantedItemInstance() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["Annotation"], Annotation);
                FromJsonUtilS(input["BundleContents"], BundleContents);
                FromJsonUtilS(input["BundleParent"], BundleParent);
                FromJsonUtilS(input["CatalogVersion"], CatalogVersion);
                FromJsonUtilS(input["CharacterId"], CharacterId);
                FromJsonUtilS(input["CustomData"], CustomData);
                FromJsonUtilS(input["DisplayName"], DisplayName);
                FromJsonUtilT(input["Expiration"], Expiration);
                FromJsonUtilS(input["ItemClass"], ItemClass);
                FromJsonUtilS(input["ItemId"], ItemId);
                FromJsonUtilS(input["ItemInstanceId"], ItemInstanceId);
                FromJsonUtilS(input["PlayFabId"], PlayFabId);
                FromJsonUtilT(input["PurchaseDate"], PurchaseDate);
                FromJsonUtilP(input["RemainingUses"], RemainingUses);
                FromJsonUtilP(input["Result"], Result);
                FromJsonUtilS(input["UnitCurrency"], UnitCurrency);
                FromJsonUtilP(input["UnitPrice"], UnitPrice);
                FromJsonUtilP(input["UsesIncrementedBy"], UsesIncrementedBy);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_Annotation; ToJsonUtilS(Annotation, each_Annotation); output["Annotation"] = each_Annotation;
                Json::Value each_BundleContents; ToJsonUtilS(BundleContents, each_BundleContents); output["BundleContents"] = each_BundleContents;
                Json::Value each_BundleParent; ToJsonUtilS(BundleParent, each_BundleParent); output["BundleParent"] = each_BundleParent;
                Json::Value each_CatalogVersion; ToJsonUtilS(CatalogVersion, each_CatalogVersion); output["CatalogVersion"] = each_CatalogVersion;
                Json::Value each_CharacterId; ToJsonUtilS(CharacterId, each_CharacterId); output["CharacterId"] = each_CharacterId;
                Json::Value each_CustomData; ToJsonUtilS(CustomData, each_CustomData); output["CustomData"] = each_CustomData;
                Json::Value each_DisplayName; ToJsonUtilS(DisplayName, each_DisplayName); output["DisplayName"] = each_DisplayName;
                Json::Value each_Expiration; ToJsonUtilT(Expiration, each_Expiration); output["Expiration"] = each_Expiration;
                Json::Value each_ItemClass; ToJsonUtilS(ItemClass, each_ItemClass); output["ItemClass"] = each_ItemClass;
                Json::Value each_ItemId; ToJsonUtilS(ItemId, each_ItemId); output["ItemId"] = each_ItemId;
                Json::Value each_ItemInstanceId; ToJsonUtilS(ItemInstanceId, each_ItemInstanceId); output["ItemInstanceId"] = each_ItemInstanceId;
                Json::Value each_PlayFabId; ToJsonUtilS(PlayFabId, each_PlayFabId); output["PlayFabId"] = each_PlayFabId;
                Json::Value each_PurchaseDate; ToJsonUtilT(PurchaseDate, each_PurchaseDate); output["PurchaseDate"] = each_PurchaseDate;
                Json::Value each_RemainingUses; ToJsonUtilP(RemainingUses, each_RemainingUses); output["RemainingUses"] = each_RemainingUses;
                Json::Value each_Result; ToJsonUtilP(Result, each_Result); output["Result"] = each_Result;
                Json::Value each_UnitCurrency; ToJsonUtilS(UnitCurrency, each_UnitCurrency); output["UnitCurrency"] = each_UnitCurrency;
                Json::Value each_UnitPrice; ToJsonUtilP(UnitPrice, each_UnitPrice); output["UnitPrice"] = each_UnitPrice;
                Json::Value each_UsesIncrementedBy; ToJsonUtilP(UsesIncrementedBy, each_UsesIncrementedBy); output["UsesIncrementedBy"] = each_UsesIncrementedBy;
                return output;
            }
        };

        struct ItemGrant : public PlayFabBaseModel
        {
            std::string Annotation;
            std::string CharacterId;
            std::map<std::string, std::string> Data;
            std::string ItemId;
            std::list<std::string> KeysToRemove;
            std::string PlayFabId;

            ItemGrant() :
                PlayFabBaseModel(),
                Annotation(),
                CharacterId(),
                Data(),
                ItemId(),
                KeysToRemove(),
                PlayFabId()
            {}

            ItemGrant(const ItemGrant& src) :
                PlayFabBaseModel(),
                Annotation(src.Annotation),
                CharacterId(src.CharacterId),
                Data(src.Data),
                ItemId(src.ItemId),
                KeysToRemove(src.KeysToRemove),
                PlayFabId(src.PlayFabId)
            {}

            ~ItemGrant() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["Annotation"], Annotation);
                FromJsonUtilS(input["CharacterId"], CharacterId);
                FromJsonUtilS(input["Data"], Data);
                FromJsonUtilS(input["ItemId"], ItemId);
                FromJsonUtilS(input["KeysToRemove"], KeysToRemove);
                FromJsonUtilS(input["PlayFabId"], PlayFabId);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_Annotation; ToJsonUtilS(Annotation, each_Annotation); output["Annotation"] = each_Annotation;
                Json::Value each_CharacterId; ToJsonUtilS(CharacterId, each_CharacterId); output["CharacterId"] = each_CharacterId;
                Json::Value each_Data; ToJsonUtilS(Data, each_Data); output["Data"] = each_Data;
                Json::Value each_ItemId; ToJsonUtilS(ItemId, each_ItemId); output["ItemId"] = each_ItemId;
                Json::Value each_KeysToRemove; ToJsonUtilS(KeysToRemove, each_KeysToRemove); output["KeysToRemove"] = each_KeysToRemove;
                Json::Value each_PlayFabId; ToJsonUtilS(PlayFabId, each_PlayFabId); output["PlayFabId"] = each_PlayFabId;
                return output;
            }
        };

        struct GrantItemsToUsersRequest : public PlayFabRequestCommon
        {
            std::string CatalogVersion;
            std::map<std::string, std::string> CustomTags;
            std::list<ItemGrant> ItemGrants;

            GrantItemsToUsersRequest() :
                PlayFabRequestCommon(),
                CatalogVersion(),
                CustomTags(),
                ItemGrants()
            {}

            GrantItemsToUsersRequest(const GrantItemsToUsersRequest& src) :
                PlayFabRequestCommon(),
                CatalogVersion(src.CatalogVersion),
                CustomTags(src.CustomTags),
                ItemGrants(src.ItemGrants)
            {}

            ~GrantItemsToUsersRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["CatalogVersion"], CatalogVersion);
                FromJsonUtilS(input["CustomTags"], CustomTags);
                FromJsonUtilO(input["ItemGrants"], ItemGrants);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_CatalogVersion; ToJsonUtilS(CatalogVersion, each_CatalogVersion); output["CatalogVersion"] = each_CatalogVersion;
                Json::Value each_CustomTags; ToJsonUtilS(CustomTags, each_CustomTags); output["CustomTags"] = each_CustomTags;
                Json::Value each_ItemGrants; ToJsonUtilO(ItemGrants, each_ItemGrants); output["ItemGrants"] = each_ItemGrants;
                return output;
            }
        };

        struct GrantItemsToUsersResult : public PlayFabResultCommon
        {
            std::list<GrantedItemInstance> ItemGrantResults;

            GrantItemsToUsersResult() :
                PlayFabResultCommon(),
                ItemGrantResults()
            {}

            GrantItemsToUsersResult(const GrantItemsToUsersResult& src) :
                PlayFabResultCommon(),
                ItemGrantResults(src.ItemGrantResults)
            {}

            ~GrantItemsToUsersResult() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilO(input["ItemGrantResults"], ItemGrantResults);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_ItemGrantResults; ToJsonUtilO(ItemGrantResults, each_ItemGrantResults); output["ItemGrantResults"] = each_ItemGrantResults;
                return output;
            }
        };

        struct IncrementLimitedEditionItemAvailabilityRequest : public PlayFabRequestCommon
        {
            Int32 Amount;
            std::string CatalogVersion;
            std::map<std::string, std::string> CustomTags;
            std::string ItemId;

            IncrementLimitedEditionItemAvailabilityRequest() :
                PlayFabRequestCommon(),
                Amount(),
                CatalogVersion(),
                CustomTags(),
                ItemId()
            {}

            IncrementLimitedEditionItemAvailabilityRequest(const IncrementLimitedEditionItemAvailabilityRequest& src) :
                PlayFabRequestCommon(),
                Amount(src.Amount),
                CatalogVersion(src.CatalogVersion),
                CustomTags(src.CustomTags),
                ItemId(src.ItemId)
            {}

            ~IncrementLimitedEditionItemAvailabilityRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilP(input["Amount"], Amount);
                FromJsonUtilS(input["CatalogVersion"], CatalogVersion);
                FromJsonUtilS(input["CustomTags"], CustomTags);
                FromJsonUtilS(input["ItemId"], ItemId);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_Amount; ToJsonUtilP(Amount, each_Amount); output["Amount"] = each_Amount;
                Json::Value each_CatalogVersion; ToJsonUtilS(CatalogVersion, each_CatalogVersion); output["CatalogVersion"] = each_CatalogVersion;
                Json::Value each_CustomTags; ToJsonUtilS(CustomTags, each_CustomTags); output["CustomTags"] = each_CustomTags;
                Json::Value each_ItemId; ToJsonUtilS(ItemId, each_ItemId); output["ItemId"] = each_ItemId;
                return output;
            }
        };

        struct IncrementLimitedEditionItemAvailabilityResult : public PlayFabResultCommon
        {

            IncrementLimitedEditionItemAvailabilityResult() :
                PlayFabResultCommon()
            {}

            IncrementLimitedEditionItemAvailabilityResult(const IncrementLimitedEditionItemAvailabilityResult&) :
                PlayFabResultCommon()
            {}

            ~IncrementLimitedEditionItemAvailabilityResult() = default;

            void FromJson(const Json::Value&) override
            {
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                return output;
            }
        };

        struct IncrementPlayerStatisticVersionRequest : public PlayFabRequestCommon
        {
            std::map<std::string, std::string> CustomTags;
            std::string StatisticName;

            IncrementPlayerStatisticVersionRequest() :
                PlayFabRequestCommon(),
                CustomTags(),
                StatisticName()
            {}

            IncrementPlayerStatisticVersionRequest(const IncrementPlayerStatisticVersionRequest& src) :
                PlayFabRequestCommon(),
                CustomTags(src.CustomTags),
                StatisticName(src.StatisticName)
            {}

            ~IncrementPlayerStatisticVersionRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["CustomTags"], CustomTags);
                FromJsonUtilS(input["StatisticName"], StatisticName);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_CustomTags; ToJsonUtilS(CustomTags, each_CustomTags); output["CustomTags"] = each_CustomTags;
                Json::Value each_StatisticName; ToJsonUtilS(StatisticName, each_StatisticName); output["StatisticName"] = each_StatisticName;
                return output;
            }
        };

        struct IncrementPlayerStatisticVersionResult : public PlayFabResultCommon
        {
            Boxed<PlayerStatisticVersion> StatisticVersion;

            IncrementPlayerStatisticVersionResult() :
                PlayFabResultCommon(),
                StatisticVersion()
            {}

            IncrementPlayerStatisticVersionResult(const IncrementPlayerStatisticVersionResult& src) :
                PlayFabResultCommon(),
                StatisticVersion(src.StatisticVersion)
            {}

            ~IncrementPlayerStatisticVersionResult() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilO(input["StatisticVersion"], StatisticVersion);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_StatisticVersion; ToJsonUtilO(StatisticVersion, each_StatisticVersion); output["StatisticVersion"] = each_StatisticVersion;
                return output;
            }
        };

        struct ListBuildsRequest : public PlayFabRequestCommon
        {

            ListBuildsRequest() :
                PlayFabRequestCommon()
            {}

            ListBuildsRequest(const ListBuildsRequest&) :
                PlayFabRequestCommon()
            {}

            ~ListBuildsRequest() = default;

            void FromJson(const Json::Value&) override
            {
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                return output;
            }
        };

        struct ListBuildsResult : public PlayFabResultCommon
        {
            std::list<GetServerBuildInfoResult> Builds;

            ListBuildsResult() :
                PlayFabResultCommon(),
                Builds()
            {}

            ListBuildsResult(const ListBuildsResult& src) :
                PlayFabResultCommon(),
                Builds(src.Builds)
            {}

            ~ListBuildsResult() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilO(input["Builds"], Builds);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_Builds; ToJsonUtilO(Builds, each_Builds); output["Builds"] = each_Builds;
                return output;
            }
        };

        struct ListOpenIdConnectionRequest : public PlayFabRequestCommon
        {

            ListOpenIdConnectionRequest() :
                PlayFabRequestCommon()
            {}

            ListOpenIdConnectionRequest(const ListOpenIdConnectionRequest&) :
                PlayFabRequestCommon()
            {}

            ~ListOpenIdConnectionRequest() = default;

            void FromJson(const Json::Value&) override
            {
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                return output;
            }
        };

        struct OpenIdConnection : public PlayFabBaseModel
        {
            std::string ClientId;
            std::string ClientSecret;
            std::string ConnectionId;
            bool DiscoverConfiguration;
            Boxed<OpenIdIssuerInformation> IssuerInformation;

            OpenIdConnection() :
                PlayFabBaseModel(),
                ClientId(),
                ClientSecret(),
                ConnectionId(),
                DiscoverConfiguration(),
                IssuerInformation()
            {}

            OpenIdConnection(const OpenIdConnection& src) :
                PlayFabBaseModel(),
                ClientId(src.ClientId),
                ClientSecret(src.ClientSecret),
                ConnectionId(src.ConnectionId),
                DiscoverConfiguration(src.DiscoverConfiguration),
                IssuerInformation(src.IssuerInformation)
            {}

            ~OpenIdConnection() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["ClientId"], ClientId);
                FromJsonUtilS(input["ClientSecret"], ClientSecret);
                FromJsonUtilS(input["ConnectionId"], ConnectionId);
                FromJsonUtilP(input["DiscoverConfiguration"], DiscoverConfiguration);
                FromJsonUtilO(input["IssuerInformation"], IssuerInformation);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_ClientId; ToJsonUtilS(ClientId, each_ClientId); output["ClientId"] = each_ClientId;
                Json::Value each_ClientSecret; ToJsonUtilS(ClientSecret, each_ClientSecret); output["ClientSecret"] = each_ClientSecret;
                Json::Value each_ConnectionId; ToJsonUtilS(ConnectionId, each_ConnectionId); output["ConnectionId"] = each_ConnectionId;
                Json::Value each_DiscoverConfiguration; ToJsonUtilP(DiscoverConfiguration, each_DiscoverConfiguration); output["DiscoverConfiguration"] = each_DiscoverConfiguration;
                Json::Value each_IssuerInformation; ToJsonUtilO(IssuerInformation, each_IssuerInformation); output["IssuerInformation"] = each_IssuerInformation;
                return output;
            }
        };

        struct ListOpenIdConnectionResponse : public PlayFabResultCommon
        {
            std::list<OpenIdConnection> Connections;

            ListOpenIdConnectionResponse() :
                PlayFabResultCommon(),
                Connections()
            {}

            ListOpenIdConnectionResponse(const ListOpenIdConnectionResponse& src) :
                PlayFabResultCommon(),
                Connections(src.Connections)
            {}

            ~ListOpenIdConnectionResponse() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilO(input["Connections"], Connections);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_Connections; ToJsonUtilO(Connections, each_Connections); output["Connections"] = each_Connections;
                return output;
            }
        };

        struct ListVirtualCurrencyTypesRequest : public PlayFabRequestCommon
        {

            ListVirtualCurrencyTypesRequest() :
                PlayFabRequestCommon()
            {}

            ListVirtualCurrencyTypesRequest(const ListVirtualCurrencyTypesRequest&) :
                PlayFabRequestCommon()
            {}

            ~ListVirtualCurrencyTypesRequest() = default;

            void FromJson(const Json::Value&) override
            {
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                return output;
            }
        };

        struct ListVirtualCurrencyTypesResult : public PlayFabResultCommon
        {
            std::list<VirtualCurrencyData> VirtualCurrencies;

            ListVirtualCurrencyTypesResult() :
                PlayFabResultCommon(),
                VirtualCurrencies()
            {}

            ListVirtualCurrencyTypesResult(const ListVirtualCurrencyTypesResult& src) :
                PlayFabResultCommon(),
                VirtualCurrencies(src.VirtualCurrencies)
            {}

            ~ListVirtualCurrencyTypesResult() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilO(input["VirtualCurrencies"], VirtualCurrencies);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_VirtualCurrencies; ToJsonUtilO(VirtualCurrencies, each_VirtualCurrencies); output["VirtualCurrencies"] = each_VirtualCurrencies;
                return output;
            }
        };

        struct LookupUserAccountInfoRequest : public PlayFabRequestCommon
        {
            std::string Email;
            std::string PlayFabId;
            std::string TitleDisplayName;
            std::string Username;

            LookupUserAccountInfoRequest() :
                PlayFabRequestCommon(),
                Email(),
                PlayFabId(),
                TitleDisplayName(),
                Username()
            {}

            LookupUserAccountInfoRequest(const LookupUserAccountInfoRequest& src) :
                PlayFabRequestCommon(),
                Email(src.Email),
                PlayFabId(src.PlayFabId),
                TitleDisplayName(src.TitleDisplayName),
                Username(src.Username)
            {}

            ~LookupUserAccountInfoRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["Email"], Email);
                FromJsonUtilS(input["PlayFabId"], PlayFabId);
                FromJsonUtilS(input["TitleDisplayName"], TitleDisplayName);
                FromJsonUtilS(input["Username"], Username);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_Email; ToJsonUtilS(Email, each_Email); output["Email"] = each_Email;
                Json::Value each_PlayFabId; ToJsonUtilS(PlayFabId, each_PlayFabId); output["PlayFabId"] = each_PlayFabId;
                Json::Value each_TitleDisplayName; ToJsonUtilS(TitleDisplayName, each_TitleDisplayName); output["TitleDisplayName"] = each_TitleDisplayName;
                Json::Value each_Username; ToJsonUtilS(Username, each_Username); output["Username"] = each_Username;
                return output;
            }
        };

        struct UserAndroidDeviceInfo : public PlayFabBaseModel
        {
            std::string AndroidDeviceId;

            UserAndroidDeviceInfo() :
                PlayFabBaseModel(),
                AndroidDeviceId()
            {}

            UserAndroidDeviceInfo(const UserAndroidDeviceInfo& src) :
                PlayFabBaseModel(),
                AndroidDeviceId(src.AndroidDeviceId)
            {}

            ~UserAndroidDeviceInfo() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["AndroidDeviceId"], AndroidDeviceId);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_AndroidDeviceId; ToJsonUtilS(AndroidDeviceId, each_AndroidDeviceId); output["AndroidDeviceId"] = each_AndroidDeviceId;
                return output;
            }
        };

        struct UserAppleIdInfo : public PlayFabBaseModel
        {
            std::string AppleSubjectId;

            UserAppleIdInfo() :
                PlayFabBaseModel(),
                AppleSubjectId()
            {}

            UserAppleIdInfo(const UserAppleIdInfo& src) :
                PlayFabBaseModel(),
                AppleSubjectId(src.AppleSubjectId)
            {}

            ~UserAppleIdInfo() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["AppleSubjectId"], AppleSubjectId);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_AppleSubjectId; ToJsonUtilS(AppleSubjectId, each_AppleSubjectId); output["AppleSubjectId"] = each_AppleSubjectId;
                return output;
            }
        };

        struct UserCustomIdInfo : public PlayFabBaseModel
        {
            std::string CustomId;

            UserCustomIdInfo() :
                PlayFabBaseModel(),
                CustomId()
            {}

            UserCustomIdInfo(const UserCustomIdInfo& src) :
                PlayFabBaseModel(),
                CustomId(src.CustomId)
            {}

            ~UserCustomIdInfo() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["CustomId"], CustomId);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_CustomId; ToJsonUtilS(CustomId, each_CustomId); output["CustomId"] = each_CustomId;
                return output;
            }
        };

        struct UserFacebookInfo : public PlayFabBaseModel
        {
            std::string FacebookId;
            std::string FullName;

            UserFacebookInfo() :
                PlayFabBaseModel(),
                FacebookId(),
                FullName()
            {}

            UserFacebookInfo(const UserFacebookInfo& src) :
                PlayFabBaseModel(),
                FacebookId(src.FacebookId),
                FullName(src.FullName)
            {}

            ~UserFacebookInfo() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["FacebookId"], FacebookId);
                FromJsonUtilS(input["FullName"], FullName);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_FacebookId; ToJsonUtilS(FacebookId, each_FacebookId); output["FacebookId"] = each_FacebookId;
                Json::Value each_FullName; ToJsonUtilS(FullName, each_FullName); output["FullName"] = each_FullName;
                return output;
            }
        };

        struct UserFacebookInstantGamesIdInfo : public PlayFabBaseModel
        {
            std::string FacebookInstantGamesId;

            UserFacebookInstantGamesIdInfo() :
                PlayFabBaseModel(),
                FacebookInstantGamesId()
            {}

            UserFacebookInstantGamesIdInfo(const UserFacebookInstantGamesIdInfo& src) :
                PlayFabBaseModel(),
                FacebookInstantGamesId(src.FacebookInstantGamesId)
            {}

            ~UserFacebookInstantGamesIdInfo() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["FacebookInstantGamesId"], FacebookInstantGamesId);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_FacebookInstantGamesId; ToJsonUtilS(FacebookInstantGamesId, each_FacebookInstantGamesId); output["FacebookInstantGamesId"] = each_FacebookInstantGamesId;
                return output;
            }
        };

        struct UserGameCenterInfo : public PlayFabBaseModel
        {
            std::string GameCenterId;

            UserGameCenterInfo() :
                PlayFabBaseModel(),
                GameCenterId()
            {}

            UserGameCenterInfo(const UserGameCenterInfo& src) :
                PlayFabBaseModel(),
                GameCenterId(src.GameCenterId)
            {}

            ~UserGameCenterInfo() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["GameCenterId"], GameCenterId);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_GameCenterId; ToJsonUtilS(GameCenterId, each_GameCenterId); output["GameCenterId"] = each_GameCenterId;
                return output;
            }
        };

        struct UserGoogleInfo : public PlayFabBaseModel
        {
            std::string GoogleEmail;
            std::string GoogleGender;
            std::string GoogleId;
            std::string GoogleLocale;
            std::string GoogleName;

            UserGoogleInfo() :
                PlayFabBaseModel(),
                GoogleEmail(),
                GoogleGender(),
                GoogleId(),
                GoogleLocale(),
                GoogleName()
            {}

            UserGoogleInfo(const UserGoogleInfo& src) :
                PlayFabBaseModel(),
                GoogleEmail(src.GoogleEmail),
                GoogleGender(src.GoogleGender),
                GoogleId(src.GoogleId),
                GoogleLocale(src.GoogleLocale),
                GoogleName(src.GoogleName)
            {}

            ~UserGoogleInfo() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["GoogleEmail"], GoogleEmail);
                FromJsonUtilS(input["GoogleGender"], GoogleGender);
                FromJsonUtilS(input["GoogleId"], GoogleId);
                FromJsonUtilS(input["GoogleLocale"], GoogleLocale);
                FromJsonUtilS(input["GoogleName"], GoogleName);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_GoogleEmail; ToJsonUtilS(GoogleEmail, each_GoogleEmail); output["GoogleEmail"] = each_GoogleEmail;
                Json::Value each_GoogleGender; ToJsonUtilS(GoogleGender, each_GoogleGender); output["GoogleGender"] = each_GoogleGender;
                Json::Value each_GoogleId; ToJsonUtilS(GoogleId, each_GoogleId); output["GoogleId"] = each_GoogleId;
                Json::Value each_GoogleLocale; ToJsonUtilS(GoogleLocale, each_GoogleLocale); output["GoogleLocale"] = each_GoogleLocale;
                Json::Value each_GoogleName; ToJsonUtilS(GoogleName, each_GoogleName); output["GoogleName"] = each_GoogleName;
                return output;
            }
        };

        struct UserIosDeviceInfo : public PlayFabBaseModel
        {
            std::string IosDeviceId;

            UserIosDeviceInfo() :
                PlayFabBaseModel(),
                IosDeviceId()
            {}

            UserIosDeviceInfo(const UserIosDeviceInfo& src) :
                PlayFabBaseModel(),
                IosDeviceId(src.IosDeviceId)
            {}

            ~UserIosDeviceInfo() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["IosDeviceId"], IosDeviceId);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_IosDeviceId; ToJsonUtilS(IosDeviceId, each_IosDeviceId); output["IosDeviceId"] = each_IosDeviceId;
                return output;
            }
        };

        struct UserKongregateInfo : public PlayFabBaseModel
        {
            std::string KongregateId;
            std::string KongregateName;

            UserKongregateInfo() :
                PlayFabBaseModel(),
                KongregateId(),
                KongregateName()
            {}

            UserKongregateInfo(const UserKongregateInfo& src) :
                PlayFabBaseModel(),
                KongregateId(src.KongregateId),
                KongregateName(src.KongregateName)
            {}

            ~UserKongregateInfo() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["KongregateId"], KongregateId);
                FromJsonUtilS(input["KongregateName"], KongregateName);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_KongregateId; ToJsonUtilS(KongregateId, each_KongregateId); output["KongregateId"] = each_KongregateId;
                Json::Value each_KongregateName; ToJsonUtilS(KongregateName, each_KongregateName); output["KongregateName"] = each_KongregateName;
                return output;
            }
        };

        struct UserNintendoSwitchAccountIdInfo : public PlayFabBaseModel
        {
            std::string NintendoSwitchAccountSubjectId;

            UserNintendoSwitchAccountIdInfo() :
                PlayFabBaseModel(),
                NintendoSwitchAccountSubjectId()
            {}

            UserNintendoSwitchAccountIdInfo(const UserNintendoSwitchAccountIdInfo& src) :
                PlayFabBaseModel(),
                NintendoSwitchAccountSubjectId(src.NintendoSwitchAccountSubjectId)
            {}

            ~UserNintendoSwitchAccountIdInfo() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["NintendoSwitchAccountSubjectId"], NintendoSwitchAccountSubjectId);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_NintendoSwitchAccountSubjectId; ToJsonUtilS(NintendoSwitchAccountSubjectId, each_NintendoSwitchAccountSubjectId); output["NintendoSwitchAccountSubjectId"] = each_NintendoSwitchAccountSubjectId;
                return output;
            }
        };

        struct UserNintendoSwitchDeviceIdInfo : public PlayFabBaseModel
        {
            std::string NintendoSwitchDeviceId;

            UserNintendoSwitchDeviceIdInfo() :
                PlayFabBaseModel(),
                NintendoSwitchDeviceId()
            {}

            UserNintendoSwitchDeviceIdInfo(const UserNintendoSwitchDeviceIdInfo& src) :
                PlayFabBaseModel(),
                NintendoSwitchDeviceId(src.NintendoSwitchDeviceId)
            {}

            ~UserNintendoSwitchDeviceIdInfo() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["NintendoSwitchDeviceId"], NintendoSwitchDeviceId);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_NintendoSwitchDeviceId; ToJsonUtilS(NintendoSwitchDeviceId, each_NintendoSwitchDeviceId); output["NintendoSwitchDeviceId"] = each_NintendoSwitchDeviceId;
                return output;
            }
        };

        struct UserOpenIdInfo : public PlayFabBaseModel
        {
            std::string ConnectionId;
            std::string Issuer;
            std::string Subject;

            UserOpenIdInfo() :
                PlayFabBaseModel(),
                ConnectionId(),
                Issuer(),
                Subject()
            {}

            UserOpenIdInfo(const UserOpenIdInfo& src) :
                PlayFabBaseModel(),
                ConnectionId(src.ConnectionId),
                Issuer(src.Issuer),
                Subject(src.Subject)
            {}

            ~UserOpenIdInfo() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["ConnectionId"], ConnectionId);
                FromJsonUtilS(input["Issuer"], Issuer);
                FromJsonUtilS(input["Subject"], Subject);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_ConnectionId; ToJsonUtilS(ConnectionId, each_ConnectionId); output["ConnectionId"] = each_ConnectionId;
                Json::Value each_Issuer; ToJsonUtilS(Issuer, each_Issuer); output["Issuer"] = each_Issuer;
                Json::Value each_Subject; ToJsonUtilS(Subject, each_Subject); output["Subject"] = each_Subject;
                return output;
            }
        };

        struct UserPrivateAccountInfo : public PlayFabBaseModel
        {
            std::string Email;

            UserPrivateAccountInfo() :
                PlayFabBaseModel(),
                Email()
            {}

            UserPrivateAccountInfo(const UserPrivateAccountInfo& src) :
                PlayFabBaseModel(),
                Email(src.Email)
            {}

            ~UserPrivateAccountInfo() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["Email"], Email);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_Email; ToJsonUtilS(Email, each_Email); output["Email"] = each_Email;
                return output;
            }
        };

        struct UserPsnInfo : public PlayFabBaseModel
        {
            std::string PsnAccountId;
            std::string PsnOnlineId;

            UserPsnInfo() :
                PlayFabBaseModel(),
                PsnAccountId(),
                PsnOnlineId()
            {}

            UserPsnInfo(const UserPsnInfo& src) :
                PlayFabBaseModel(),
                PsnAccountId(src.PsnAccountId),
                PsnOnlineId(src.PsnOnlineId)
            {}

            ~UserPsnInfo() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["PsnAccountId"], PsnAccountId);
                FromJsonUtilS(input["PsnOnlineId"], PsnOnlineId);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_PsnAccountId; ToJsonUtilS(PsnAccountId, each_PsnAccountId); output["PsnAccountId"] = each_PsnAccountId;
                Json::Value each_PsnOnlineId; ToJsonUtilS(PsnOnlineId, each_PsnOnlineId); output["PsnOnlineId"] = each_PsnOnlineId;
                return output;
            }
        };

        struct UserSteamInfo : public PlayFabBaseModel
        {
            Boxed<TitleActivationStatus> SteamActivationStatus;
            std::string SteamCountry;
            Boxed<Currency> SteamCurrency;
            std::string SteamId;
            std::string SteamName;

            UserSteamInfo() :
                PlayFabBaseModel(),
                SteamActivationStatus(),
                SteamCountry(),
                SteamCurrency(),
                SteamId(),
                SteamName()
            {}

            UserSteamInfo(const UserSteamInfo& src) :
                PlayFabBaseModel(),
                SteamActivationStatus(src.SteamActivationStatus),
                SteamCountry(src.SteamCountry),
                SteamCurrency(src.SteamCurrency),
                SteamId(src.SteamId),
                SteamName(src.SteamName)
            {}

            ~UserSteamInfo() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilE(input["SteamActivationStatus"], SteamActivationStatus);
                FromJsonUtilS(input["SteamCountry"], SteamCountry);
                FromJsonUtilE(input["SteamCurrency"], SteamCurrency);
                FromJsonUtilS(input["SteamId"], SteamId);
                FromJsonUtilS(input["SteamName"], SteamName);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_SteamActivationStatus; ToJsonUtilE(SteamActivationStatus, each_SteamActivationStatus); output["SteamActivationStatus"] = each_SteamActivationStatus;
                Json::Value each_SteamCountry; ToJsonUtilS(SteamCountry, each_SteamCountry); output["SteamCountry"] = each_SteamCountry;
                Json::Value each_SteamCurrency; ToJsonUtilE(SteamCurrency, each_SteamCurrency); output["SteamCurrency"] = each_SteamCurrency;
                Json::Value each_SteamId; ToJsonUtilS(SteamId, each_SteamId); output["SteamId"] = each_SteamId;
                Json::Value each_SteamName; ToJsonUtilS(SteamName, each_SteamName); output["SteamName"] = each_SteamName;
                return output;
            }
        };

        struct UserTitleInfo : public PlayFabBaseModel
        {
            std::string AvatarUrl;
            time_t Created;
            std::string DisplayName;
            Boxed<time_t> FirstLogin;
            Boxed<bool> isBanned;
            Boxed<time_t> LastLogin;
            Boxed<UserOrigination> Origination;
            Boxed<EntityKey> TitlePlayerAccount;

            UserTitleInfo() :
                PlayFabBaseModel(),
                AvatarUrl(),
                Created(),
                DisplayName(),
                FirstLogin(),
                isBanned(),
                LastLogin(),
                Origination(),
                TitlePlayerAccount()
            {}

            UserTitleInfo(const UserTitleInfo& src) :
                PlayFabBaseModel(),
                AvatarUrl(src.AvatarUrl),
                Created(src.Created),
                DisplayName(src.DisplayName),
                FirstLogin(src.FirstLogin),
                isBanned(src.isBanned),
                LastLogin(src.LastLogin),
                Origination(src.Origination),
                TitlePlayerAccount(src.TitlePlayerAccount)
            {}

            ~UserTitleInfo() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["AvatarUrl"], AvatarUrl);
                FromJsonUtilT(input["Created"], Created);
                FromJsonUtilS(input["DisplayName"], DisplayName);
                FromJsonUtilT(input["FirstLogin"], FirstLogin);
                FromJsonUtilP(input["isBanned"], isBanned);
                FromJsonUtilT(input["LastLogin"], LastLogin);
                FromJsonUtilE(input["Origination"], Origination);
                FromJsonUtilO(input["TitlePlayerAccount"], TitlePlayerAccount);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_AvatarUrl; ToJsonUtilS(AvatarUrl, each_AvatarUrl); output["AvatarUrl"] = each_AvatarUrl;
                Json::Value each_Created; ToJsonUtilT(Created, each_Created); output["Created"] = each_Created;
                Json::Value each_DisplayName; ToJsonUtilS(DisplayName, each_DisplayName); output["DisplayName"] = each_DisplayName;
                Json::Value each_FirstLogin; ToJsonUtilT(FirstLogin, each_FirstLogin); output["FirstLogin"] = each_FirstLogin;
                Json::Value each_isBanned; ToJsonUtilP(isBanned, each_isBanned); output["isBanned"] = each_isBanned;
                Json::Value each_LastLogin; ToJsonUtilT(LastLogin, each_LastLogin); output["LastLogin"] = each_LastLogin;
                Json::Value each_Origination; ToJsonUtilE(Origination, each_Origination); output["Origination"] = each_Origination;
                Json::Value each_TitlePlayerAccount; ToJsonUtilO(TitlePlayerAccount, each_TitlePlayerAccount); output["TitlePlayerAccount"] = each_TitlePlayerAccount;
                return output;
            }
        };

        struct UserTwitchInfo : public PlayFabBaseModel
        {
            std::string TwitchId;
            std::string TwitchUserName;

            UserTwitchInfo() :
                PlayFabBaseModel(),
                TwitchId(),
                TwitchUserName()
            {}

            UserTwitchInfo(const UserTwitchInfo& src) :
                PlayFabBaseModel(),
                TwitchId(src.TwitchId),
                TwitchUserName(src.TwitchUserName)
            {}

            ~UserTwitchInfo() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["TwitchId"], TwitchId);
                FromJsonUtilS(input["TwitchUserName"], TwitchUserName);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_TwitchId; ToJsonUtilS(TwitchId, each_TwitchId); output["TwitchId"] = each_TwitchId;
                Json::Value each_TwitchUserName; ToJsonUtilS(TwitchUserName, each_TwitchUserName); output["TwitchUserName"] = each_TwitchUserName;
                return output;
            }
        };

        struct UserWindowsHelloInfo : public PlayFabBaseModel
        {
            std::string WindowsHelloDeviceName;
            std::string WindowsHelloPublicKeyHash;

            UserWindowsHelloInfo() :
                PlayFabBaseModel(),
                WindowsHelloDeviceName(),
                WindowsHelloPublicKeyHash()
            {}

            UserWindowsHelloInfo(const UserWindowsHelloInfo& src) :
                PlayFabBaseModel(),
                WindowsHelloDeviceName(src.WindowsHelloDeviceName),
                WindowsHelloPublicKeyHash(src.WindowsHelloPublicKeyHash)
            {}

            ~UserWindowsHelloInfo() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["WindowsHelloDeviceName"], WindowsHelloDeviceName);
                FromJsonUtilS(input["WindowsHelloPublicKeyHash"], WindowsHelloPublicKeyHash);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_WindowsHelloDeviceName; ToJsonUtilS(WindowsHelloDeviceName, each_WindowsHelloDeviceName); output["WindowsHelloDeviceName"] = each_WindowsHelloDeviceName;
                Json::Value each_WindowsHelloPublicKeyHash; ToJsonUtilS(WindowsHelloPublicKeyHash, each_WindowsHelloPublicKeyHash); output["WindowsHelloPublicKeyHash"] = each_WindowsHelloPublicKeyHash;
                return output;
            }
        };

        struct UserXboxInfo : public PlayFabBaseModel
        {
            std::string XboxUserId;

            UserXboxInfo() :
                PlayFabBaseModel(),
                XboxUserId()
            {}

            UserXboxInfo(const UserXboxInfo& src) :
                PlayFabBaseModel(),
                XboxUserId(src.XboxUserId)
            {}

            ~UserXboxInfo() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["XboxUserId"], XboxUserId);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_XboxUserId; ToJsonUtilS(XboxUserId, each_XboxUserId); output["XboxUserId"] = each_XboxUserId;
                return output;
            }
        };

        struct UserAccountInfo : public PlayFabBaseModel
        {
            Boxed<UserAndroidDeviceInfo> AndroidDeviceInfo;
            Boxed<UserAppleIdInfo> AppleAccountInfo;
            time_t Created;
            Boxed<UserCustomIdInfo> CustomIdInfo;
            Boxed<UserFacebookInfo> FacebookInfo;
            Boxed<UserFacebookInstantGamesIdInfo> FacebookInstantGamesIdInfo;
            Boxed<UserGameCenterInfo> GameCenterInfo;
            Boxed<UserGoogleInfo> GoogleInfo;
            Boxed<UserIosDeviceInfo> IosDeviceInfo;
            Boxed<UserKongregateInfo> KongregateInfo;
            Boxed<UserNintendoSwitchAccountIdInfo> NintendoSwitchAccountInfo;
            Boxed<UserNintendoSwitchDeviceIdInfo> NintendoSwitchDeviceIdInfo;
            std::list<UserOpenIdInfo> OpenIdInfo;
            std::string PlayFabId;
            Boxed<UserPrivateAccountInfo> PrivateInfo;
            Boxed<UserPsnInfo> PsnInfo;
            Boxed<UserSteamInfo> SteamInfo;
            Boxed<UserTitleInfo> TitleInfo;
            Boxed<UserTwitchInfo> TwitchInfo;
            std::string Username;
            Boxed<UserWindowsHelloInfo> WindowsHelloInfo;
            Boxed<UserXboxInfo> XboxInfo;

            UserAccountInfo() :
                PlayFabBaseModel(),
                AndroidDeviceInfo(),
                AppleAccountInfo(),
                Created(),
                CustomIdInfo(),
                FacebookInfo(),
                FacebookInstantGamesIdInfo(),
                GameCenterInfo(),
                GoogleInfo(),
                IosDeviceInfo(),
                KongregateInfo(),
                NintendoSwitchAccountInfo(),
                NintendoSwitchDeviceIdInfo(),
                OpenIdInfo(),
                PlayFabId(),
                PrivateInfo(),
                PsnInfo(),
                SteamInfo(),
                TitleInfo(),
                TwitchInfo(),
                Username(),
                WindowsHelloInfo(),
                XboxInfo()
            {}

            UserAccountInfo(const UserAccountInfo& src) :
                PlayFabBaseModel(),
                AndroidDeviceInfo(src.AndroidDeviceInfo),
                AppleAccountInfo(src.AppleAccountInfo),
                Created(src.Created),
                CustomIdInfo(src.CustomIdInfo),
                FacebookInfo(src.FacebookInfo),
                FacebookInstantGamesIdInfo(src.FacebookInstantGamesIdInfo),
                GameCenterInfo(src.GameCenterInfo),
                GoogleInfo(src.GoogleInfo),
                IosDeviceInfo(src.IosDeviceInfo),
                KongregateInfo(src.KongregateInfo),
                NintendoSwitchAccountInfo(src.NintendoSwitchAccountInfo),
                NintendoSwitchDeviceIdInfo(src.NintendoSwitchDeviceIdInfo),
                OpenIdInfo(src.OpenIdInfo),
                PlayFabId(src.PlayFabId),
                PrivateInfo(src.PrivateInfo),
                PsnInfo(src.PsnInfo),
                SteamInfo(src.SteamInfo),
                TitleInfo(src.TitleInfo),
                TwitchInfo(src.TwitchInfo),
                Username(src.Username),
                WindowsHelloInfo(src.WindowsHelloInfo),
                XboxInfo(src.XboxInfo)
            {}

            ~UserAccountInfo() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilO(input["AndroidDeviceInfo"], AndroidDeviceInfo);
                FromJsonUtilO(input["AppleAccountInfo"], AppleAccountInfo);
                FromJsonUtilT(input["Created"], Created);
                FromJsonUtilO(input["CustomIdInfo"], CustomIdInfo);
                FromJsonUtilO(input["FacebookInfo"], FacebookInfo);
                FromJsonUtilO(input["FacebookInstantGamesIdInfo"], FacebookInstantGamesIdInfo);
                FromJsonUtilO(input["GameCenterInfo"], GameCenterInfo);
                FromJsonUtilO(input["GoogleInfo"], GoogleInfo);
                FromJsonUtilO(input["IosDeviceInfo"], IosDeviceInfo);
                FromJsonUtilO(input["KongregateInfo"], KongregateInfo);
                FromJsonUtilO(input["NintendoSwitchAccountInfo"], NintendoSwitchAccountInfo);
                FromJsonUtilO(input["NintendoSwitchDeviceIdInfo"], NintendoSwitchDeviceIdInfo);
                FromJsonUtilO(input["OpenIdInfo"], OpenIdInfo);
                FromJsonUtilS(input["PlayFabId"], PlayFabId);
                FromJsonUtilO(input["PrivateInfo"], PrivateInfo);
                FromJsonUtilO(input["PsnInfo"], PsnInfo);
                FromJsonUtilO(input["SteamInfo"], SteamInfo);
                FromJsonUtilO(input["TitleInfo"], TitleInfo);
                FromJsonUtilO(input["TwitchInfo"], TwitchInfo);
                FromJsonUtilS(input["Username"], Username);
                FromJsonUtilO(input["WindowsHelloInfo"], WindowsHelloInfo);
                FromJsonUtilO(input["XboxInfo"], XboxInfo);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_AndroidDeviceInfo; ToJsonUtilO(AndroidDeviceInfo, each_AndroidDeviceInfo); output["AndroidDeviceInfo"] = each_AndroidDeviceInfo;
                Json::Value each_AppleAccountInfo; ToJsonUtilO(AppleAccountInfo, each_AppleAccountInfo); output["AppleAccountInfo"] = each_AppleAccountInfo;
                Json::Value each_Created; ToJsonUtilT(Created, each_Created); output["Created"] = each_Created;
                Json::Value each_CustomIdInfo; ToJsonUtilO(CustomIdInfo, each_CustomIdInfo); output["CustomIdInfo"] = each_CustomIdInfo;
                Json::Value each_FacebookInfo; ToJsonUtilO(FacebookInfo, each_FacebookInfo); output["FacebookInfo"] = each_FacebookInfo;
                Json::Value each_FacebookInstantGamesIdInfo; ToJsonUtilO(FacebookInstantGamesIdInfo, each_FacebookInstantGamesIdInfo); output["FacebookInstantGamesIdInfo"] = each_FacebookInstantGamesIdInfo;
                Json::Value each_GameCenterInfo; ToJsonUtilO(GameCenterInfo, each_GameCenterInfo); output["GameCenterInfo"] = each_GameCenterInfo;
                Json::Value each_GoogleInfo; ToJsonUtilO(GoogleInfo, each_GoogleInfo); output["GoogleInfo"] = each_GoogleInfo;
                Json::Value each_IosDeviceInfo; ToJsonUtilO(IosDeviceInfo, each_IosDeviceInfo); output["IosDeviceInfo"] = each_IosDeviceInfo;
                Json::Value each_KongregateInfo; ToJsonUtilO(KongregateInfo, each_KongregateInfo); output["KongregateInfo"] = each_KongregateInfo;
                Json::Value each_NintendoSwitchAccountInfo; ToJsonUtilO(NintendoSwitchAccountInfo, each_NintendoSwitchAccountInfo); output["NintendoSwitchAccountInfo"] = each_NintendoSwitchAccountInfo;
                Json::Value each_NintendoSwitchDeviceIdInfo; ToJsonUtilO(NintendoSwitchDeviceIdInfo, each_NintendoSwitchDeviceIdInfo); output["NintendoSwitchDeviceIdInfo"] = each_NintendoSwitchDeviceIdInfo;
                Json::Value each_OpenIdInfo; ToJsonUtilO(OpenIdInfo, each_OpenIdInfo); output["OpenIdInfo"] = each_OpenIdInfo;
                Json::Value each_PlayFabId; ToJsonUtilS(PlayFabId, each_PlayFabId); output["PlayFabId"] = each_PlayFabId;
                Json::Value each_PrivateInfo; ToJsonUtilO(PrivateInfo, each_PrivateInfo); output["PrivateInfo"] = each_PrivateInfo;
                Json::Value each_PsnInfo; ToJsonUtilO(PsnInfo, each_PsnInfo); output["PsnInfo"] = each_PsnInfo;
                Json::Value each_SteamInfo; ToJsonUtilO(SteamInfo, each_SteamInfo); output["SteamInfo"] = each_SteamInfo;
                Json::Value each_TitleInfo; ToJsonUtilO(TitleInfo, each_TitleInfo); output["TitleInfo"] = each_TitleInfo;
                Json::Value each_TwitchInfo; ToJsonUtilO(TwitchInfo, each_TwitchInfo); output["TwitchInfo"] = each_TwitchInfo;
                Json::Value each_Username; ToJsonUtilS(Username, each_Username); output["Username"] = each_Username;
                Json::Value each_WindowsHelloInfo; ToJsonUtilO(WindowsHelloInfo, each_WindowsHelloInfo); output["WindowsHelloInfo"] = each_WindowsHelloInfo;
                Json::Value each_XboxInfo; ToJsonUtilO(XboxInfo, each_XboxInfo); output["XboxInfo"] = each_XboxInfo;
                return output;
            }
        };

        struct LookupUserAccountInfoResult : public PlayFabResultCommon
        {
            Boxed<UserAccountInfo> UserInfo;

            LookupUserAccountInfoResult() :
                PlayFabResultCommon(),
                UserInfo()
            {}

            LookupUserAccountInfoResult(const LookupUserAccountInfoResult& src) :
                PlayFabResultCommon(),
                UserInfo(src.UserInfo)
            {}

            ~LookupUserAccountInfoResult() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilO(input["UserInfo"], UserInfo);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_UserInfo; ToJsonUtilO(UserInfo, each_UserInfo); output["UserInfo"] = each_UserInfo;
                return output;
            }
        };

        struct ModifyMatchmakerGameModesRequest : public PlayFabRequestCommon
        {
            std::string BuildVersion;
            std::list<GameModeInfo> GameModes;

            ModifyMatchmakerGameModesRequest() :
                PlayFabRequestCommon(),
                BuildVersion(),
                GameModes()
            {}

            ModifyMatchmakerGameModesRequest(const ModifyMatchmakerGameModesRequest& src) :
                PlayFabRequestCommon(),
                BuildVersion(src.BuildVersion),
                GameModes(src.GameModes)
            {}

            ~ModifyMatchmakerGameModesRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["BuildVersion"], BuildVersion);
                FromJsonUtilO(input["GameModes"], GameModes);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_BuildVersion; ToJsonUtilS(BuildVersion, each_BuildVersion); output["BuildVersion"] = each_BuildVersion;
                Json::Value each_GameModes; ToJsonUtilO(GameModes, each_GameModes); output["GameModes"] = each_GameModes;
                return output;
            }
        };

        struct ModifyMatchmakerGameModesResult : public PlayFabResultCommon
        {

            ModifyMatchmakerGameModesResult() :
                PlayFabResultCommon()
            {}

            ModifyMatchmakerGameModesResult(const ModifyMatchmakerGameModesResult&) :
                PlayFabResultCommon()
            {}

            ~ModifyMatchmakerGameModesResult() = default;

            void FromJson(const Json::Value&) override
            {
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                return output;
            }
        };

        struct ModifyServerBuildRequest : public PlayFabRequestCommon
        {
            std::list<Region> ActiveRegions;
            std::string BuildId;
            std::string CommandLineTemplate;
            std::string Comment;
            std::map<std::string, std::string> CustomTags;
            std::string ExecutablePath;
            Int32 MaxGamesPerHost;
            Int32 MinFreeGameSlots;
            Boxed<time_t> Timestamp;

            ModifyServerBuildRequest() :
                PlayFabRequestCommon(),
                ActiveRegions(),
                BuildId(),
                CommandLineTemplate(),
                Comment(),
                CustomTags(),
                ExecutablePath(),
                MaxGamesPerHost(),
                MinFreeGameSlots(),
                Timestamp()
            {}

            ModifyServerBuildRequest(const ModifyServerBuildRequest& src) :
                PlayFabRequestCommon(),
                ActiveRegions(src.ActiveRegions),
                BuildId(src.BuildId),
                CommandLineTemplate(src.CommandLineTemplate),
                Comment(src.Comment),
                CustomTags(src.CustomTags),
                ExecutablePath(src.ExecutablePath),
                MaxGamesPerHost(src.MaxGamesPerHost),
                MinFreeGameSlots(src.MinFreeGameSlots),
                Timestamp(src.Timestamp)
            {}

            ~ModifyServerBuildRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilE(input["ActiveRegions"], ActiveRegions);
                FromJsonUtilS(input["BuildId"], BuildId);
                FromJsonUtilS(input["CommandLineTemplate"], CommandLineTemplate);
                FromJsonUtilS(input["Comment"], Comment);
                FromJsonUtilS(input["CustomTags"], CustomTags);
                FromJsonUtilS(input["ExecutablePath"], ExecutablePath);
                FromJsonUtilP(input["MaxGamesPerHost"], MaxGamesPerHost);
                FromJsonUtilP(input["MinFreeGameSlots"], MinFreeGameSlots);
                FromJsonUtilT(input["Timestamp"], Timestamp);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_ActiveRegions; ToJsonUtilE(ActiveRegions, each_ActiveRegions); output["ActiveRegions"] = each_ActiveRegions;
                Json::Value each_BuildId; ToJsonUtilS(BuildId, each_BuildId); output["BuildId"] = each_BuildId;
                Json::Value each_CommandLineTemplate; ToJsonUtilS(CommandLineTemplate, each_CommandLineTemplate); output["CommandLineTemplate"] = each_CommandLineTemplate;
                Json::Value each_Comment; ToJsonUtilS(Comment, each_Comment); output["Comment"] = each_Comment;
                Json::Value each_CustomTags; ToJsonUtilS(CustomTags, each_CustomTags); output["CustomTags"] = each_CustomTags;
                Json::Value each_ExecutablePath; ToJsonUtilS(ExecutablePath, each_ExecutablePath); output["ExecutablePath"] = each_ExecutablePath;
                Json::Value each_MaxGamesPerHost; ToJsonUtilP(MaxGamesPerHost, each_MaxGamesPerHost); output["MaxGamesPerHost"] = each_MaxGamesPerHost;
                Json::Value each_MinFreeGameSlots; ToJsonUtilP(MinFreeGameSlots, each_MinFreeGameSlots); output["MinFreeGameSlots"] = each_MinFreeGameSlots;
                Json::Value each_Timestamp; ToJsonUtilT(Timestamp, each_Timestamp); output["Timestamp"] = each_Timestamp;
                return output;
            }
        };

        struct ModifyServerBuildResult : public PlayFabResultCommon
        {
            std::list<Region> ActiveRegions;
            std::string BuildId;
            std::string CommandLineTemplate;
            std::string Comment;
            std::string ExecutablePath;
            Int32 MaxGamesPerHost;
            Int32 MinFreeGameSlots;
            Boxed<GameBuildStatus> Status;
            time_t Timestamp;
            std::string TitleId;

            ModifyServerBuildResult() :
                PlayFabResultCommon(),
                ActiveRegions(),
                BuildId(),
                CommandLineTemplate(),
                Comment(),
                ExecutablePath(),
                MaxGamesPerHost(),
                MinFreeGameSlots(),
                Status(),
                Timestamp(),
                TitleId()
            {}

            ModifyServerBuildResult(const ModifyServerBuildResult& src) :
                PlayFabResultCommon(),
                ActiveRegions(src.ActiveRegions),
                BuildId(src.BuildId),
                CommandLineTemplate(src.CommandLineTemplate),
                Comment(src.Comment),
                ExecutablePath(src.ExecutablePath),
                MaxGamesPerHost(src.MaxGamesPerHost),
                MinFreeGameSlots(src.MinFreeGameSlots),
                Status(src.Status),
                Timestamp(src.Timestamp),
                TitleId(src.TitleId)
            {}

            ~ModifyServerBuildResult() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilE(input["ActiveRegions"], ActiveRegions);
                FromJsonUtilS(input["BuildId"], BuildId);
                FromJsonUtilS(input["CommandLineTemplate"], CommandLineTemplate);
                FromJsonUtilS(input["Comment"], Comment);
                FromJsonUtilS(input["ExecutablePath"], ExecutablePath);
                FromJsonUtilP(input["MaxGamesPerHost"], MaxGamesPerHost);
                FromJsonUtilP(input["MinFreeGameSlots"], MinFreeGameSlots);
                FromJsonUtilE(input["Status"], Status);
                FromJsonUtilT(input["Timestamp"], Timestamp);
                FromJsonUtilS(input["TitleId"], TitleId);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_ActiveRegions; ToJsonUtilE(ActiveRegions, each_ActiveRegions); output["ActiveRegions"] = each_ActiveRegions;
                Json::Value each_BuildId; ToJsonUtilS(BuildId, each_BuildId); output["BuildId"] = each_BuildId;
                Json::Value each_CommandLineTemplate; ToJsonUtilS(CommandLineTemplate, each_CommandLineTemplate); output["CommandLineTemplate"] = each_CommandLineTemplate;
                Json::Value each_Comment; ToJsonUtilS(Comment, each_Comment); output["Comment"] = each_Comment;
                Json::Value each_ExecutablePath; ToJsonUtilS(ExecutablePath, each_ExecutablePath); output["ExecutablePath"] = each_ExecutablePath;
                Json::Value each_MaxGamesPerHost; ToJsonUtilP(MaxGamesPerHost, each_MaxGamesPerHost); output["MaxGamesPerHost"] = each_MaxGamesPerHost;
                Json::Value each_MinFreeGameSlots; ToJsonUtilP(MinFreeGameSlots, each_MinFreeGameSlots); output["MinFreeGameSlots"] = each_MinFreeGameSlots;
                Json::Value each_Status; ToJsonUtilE(Status, each_Status); output["Status"] = each_Status;
                Json::Value each_Timestamp; ToJsonUtilT(Timestamp, each_Timestamp); output["Timestamp"] = each_Timestamp;
                Json::Value each_TitleId; ToJsonUtilS(TitleId, each_TitleId); output["TitleId"] = each_TitleId;
                return output;
            }
        };

        struct ModifyUserVirtualCurrencyResult : public PlayFabResultCommon
        {
            Int32 Balance;
            Int32 BalanceChange;
            std::string PlayFabId;
            std::string VirtualCurrency;

            ModifyUserVirtualCurrencyResult() :
                PlayFabResultCommon(),
                Balance(),
                BalanceChange(),
                PlayFabId(),
                VirtualCurrency()
            {}

            ModifyUserVirtualCurrencyResult(const ModifyUserVirtualCurrencyResult& src) :
                PlayFabResultCommon(),
                Balance(src.Balance),
                BalanceChange(src.BalanceChange),
                PlayFabId(src.PlayFabId),
                VirtualCurrency(src.VirtualCurrency)
            {}

            ~ModifyUserVirtualCurrencyResult() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilP(input["Balance"], Balance);
                FromJsonUtilP(input["BalanceChange"], BalanceChange);
                FromJsonUtilS(input["PlayFabId"], PlayFabId);
                FromJsonUtilS(input["VirtualCurrency"], VirtualCurrency);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_Balance; ToJsonUtilP(Balance, each_Balance); output["Balance"] = each_Balance;
                Json::Value each_BalanceChange; ToJsonUtilP(BalanceChange, each_BalanceChange); output["BalanceChange"] = each_BalanceChange;
                Json::Value each_PlayFabId; ToJsonUtilS(PlayFabId, each_PlayFabId); output["PlayFabId"] = each_PlayFabId;
                Json::Value each_VirtualCurrency; ToJsonUtilS(VirtualCurrency, each_VirtualCurrency); output["VirtualCurrency"] = each_VirtualCurrency;
                return output;
            }
        };

        struct RandomResultTable : public PlayFabBaseModel
        {
            std::list<ResultTableNode> Nodes;
            std::string TableId;

            RandomResultTable() :
                PlayFabBaseModel(),
                Nodes(),
                TableId()
            {}

            RandomResultTable(const RandomResultTable& src) :
                PlayFabBaseModel(),
                Nodes(src.Nodes),
                TableId(src.TableId)
            {}

            ~RandomResultTable() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilO(input["Nodes"], Nodes);
                FromJsonUtilS(input["TableId"], TableId);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_Nodes; ToJsonUtilO(Nodes, each_Nodes); output["Nodes"] = each_Nodes;
                Json::Value each_TableId; ToJsonUtilS(TableId, each_TableId); output["TableId"] = each_TableId;
                return output;
            }
        };

        struct RefundPurchaseRequest : public PlayFabRequestCommon
        {
            std::string OrderId;
            std::string PlayFabId;
            std::string Reason;

            RefundPurchaseRequest() :
                PlayFabRequestCommon(),
                OrderId(),
                PlayFabId(),
                Reason()
            {}

            RefundPurchaseRequest(const RefundPurchaseRequest& src) :
                PlayFabRequestCommon(),
                OrderId(src.OrderId),
                PlayFabId(src.PlayFabId),
                Reason(src.Reason)
            {}

            ~RefundPurchaseRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["OrderId"], OrderId);
                FromJsonUtilS(input["PlayFabId"], PlayFabId);
                FromJsonUtilS(input["Reason"], Reason);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_OrderId; ToJsonUtilS(OrderId, each_OrderId); output["OrderId"] = each_OrderId;
                Json::Value each_PlayFabId; ToJsonUtilS(PlayFabId, each_PlayFabId); output["PlayFabId"] = each_PlayFabId;
                Json::Value each_Reason; ToJsonUtilS(Reason, each_Reason); output["Reason"] = each_Reason;
                return output;
            }
        };

        struct RefundPurchaseResponse : public PlayFabResultCommon
        {
            std::string PurchaseStatus;

            RefundPurchaseResponse() :
                PlayFabResultCommon(),
                PurchaseStatus()
            {}

            RefundPurchaseResponse(const RefundPurchaseResponse& src) :
                PlayFabResultCommon(),
                PurchaseStatus(src.PurchaseStatus)
            {}

            ~RefundPurchaseResponse() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["PurchaseStatus"], PurchaseStatus);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_PurchaseStatus; ToJsonUtilS(PurchaseStatus, each_PurchaseStatus); output["PurchaseStatus"] = each_PurchaseStatus;
                return output;
            }
        };

        struct RemovePlayerTagRequest : public PlayFabRequestCommon
        {
            std::map<std::string, std::string> CustomTags;
            std::string PlayFabId;
            std::string TagName;

            RemovePlayerTagRequest() :
                PlayFabRequestCommon(),
                CustomTags(),
                PlayFabId(),
                TagName()
            {}

            RemovePlayerTagRequest(const RemovePlayerTagRequest& src) :
                PlayFabRequestCommon(),
                CustomTags(src.CustomTags),
                PlayFabId(src.PlayFabId),
                TagName(src.TagName)
            {}

            ~RemovePlayerTagRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["CustomTags"], CustomTags);
                FromJsonUtilS(input["PlayFabId"], PlayFabId);
                FromJsonUtilS(input["TagName"], TagName);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_CustomTags; ToJsonUtilS(CustomTags, each_CustomTags); output["CustomTags"] = each_CustomTags;
                Json::Value each_PlayFabId; ToJsonUtilS(PlayFabId, each_PlayFabId); output["PlayFabId"] = each_PlayFabId;
                Json::Value each_TagName; ToJsonUtilS(TagName, each_TagName); output["TagName"] = each_TagName;
                return output;
            }
        };

        struct RemovePlayerTagResult : public PlayFabResultCommon
        {

            RemovePlayerTagResult() :
                PlayFabResultCommon()
            {}

            RemovePlayerTagResult(const RemovePlayerTagResult&) :
                PlayFabResultCommon()
            {}

            ~RemovePlayerTagResult() = default;

            void FromJson(const Json::Value&) override
            {
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                return output;
            }
        };

        struct RemoveServerBuildRequest : public PlayFabRequestCommon
        {
            std::string BuildId;

            RemoveServerBuildRequest() :
                PlayFabRequestCommon(),
                BuildId()
            {}

            RemoveServerBuildRequest(const RemoveServerBuildRequest& src) :
                PlayFabRequestCommon(),
                BuildId(src.BuildId)
            {}

            ~RemoveServerBuildRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["BuildId"], BuildId);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_BuildId; ToJsonUtilS(BuildId, each_BuildId); output["BuildId"] = each_BuildId;
                return output;
            }
        };

        struct RemoveServerBuildResult : public PlayFabResultCommon
        {

            RemoveServerBuildResult() :
                PlayFabResultCommon()
            {}

            RemoveServerBuildResult(const RemoveServerBuildResult&) :
                PlayFabResultCommon()
            {}

            ~RemoveServerBuildResult() = default;

            void FromJson(const Json::Value&) override
            {
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                return output;
            }
        };

        struct RemoveVirtualCurrencyTypesRequest : public PlayFabRequestCommon
        {
            std::list<VirtualCurrencyData> VirtualCurrencies;

            RemoveVirtualCurrencyTypesRequest() :
                PlayFabRequestCommon(),
                VirtualCurrencies()
            {}

            RemoveVirtualCurrencyTypesRequest(const RemoveVirtualCurrencyTypesRequest& src) :
                PlayFabRequestCommon(),
                VirtualCurrencies(src.VirtualCurrencies)
            {}

            ~RemoveVirtualCurrencyTypesRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilO(input["VirtualCurrencies"], VirtualCurrencies);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_VirtualCurrencies; ToJsonUtilO(VirtualCurrencies, each_VirtualCurrencies); output["VirtualCurrencies"] = each_VirtualCurrencies;
                return output;
            }
        };

        struct ResetCharacterStatisticsRequest : public PlayFabRequestCommon
        {
            std::string CharacterId;
            std::map<std::string, std::string> CustomTags;
            std::string PlayFabId;

            ResetCharacterStatisticsRequest() :
                PlayFabRequestCommon(),
                CharacterId(),
                CustomTags(),
                PlayFabId()
            {}

            ResetCharacterStatisticsRequest(const ResetCharacterStatisticsRequest& src) :
                PlayFabRequestCommon(),
                CharacterId(src.CharacterId),
                CustomTags(src.CustomTags),
                PlayFabId(src.PlayFabId)
            {}

            ~ResetCharacterStatisticsRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["CharacterId"], CharacterId);
                FromJsonUtilS(input["CustomTags"], CustomTags);
                FromJsonUtilS(input["PlayFabId"], PlayFabId);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_CharacterId; ToJsonUtilS(CharacterId, each_CharacterId); output["CharacterId"] = each_CharacterId;
                Json::Value each_CustomTags; ToJsonUtilS(CustomTags, each_CustomTags); output["CustomTags"] = each_CustomTags;
                Json::Value each_PlayFabId; ToJsonUtilS(PlayFabId, each_PlayFabId); output["PlayFabId"] = each_PlayFabId;
                return output;
            }
        };

        struct ResetCharacterStatisticsResult : public PlayFabResultCommon
        {

            ResetCharacterStatisticsResult() :
                PlayFabResultCommon()
            {}

            ResetCharacterStatisticsResult(const ResetCharacterStatisticsResult&) :
                PlayFabResultCommon()
            {}

            ~ResetCharacterStatisticsResult() = default;

            void FromJson(const Json::Value&) override
            {
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                return output;
            }
        };

        struct ResetPasswordRequest : public PlayFabRequestCommon
        {
            std::map<std::string, std::string> CustomTags;
            std::string Password;
            std::string Token;

            ResetPasswordRequest() :
                PlayFabRequestCommon(),
                CustomTags(),
                Password(),
                Token()
            {}

            ResetPasswordRequest(const ResetPasswordRequest& src) :
                PlayFabRequestCommon(),
                CustomTags(src.CustomTags),
                Password(src.Password),
                Token(src.Token)
            {}

            ~ResetPasswordRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["CustomTags"], CustomTags);
                FromJsonUtilS(input["Password"], Password);
                FromJsonUtilS(input["Token"], Token);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_CustomTags; ToJsonUtilS(CustomTags, each_CustomTags); output["CustomTags"] = each_CustomTags;
                Json::Value each_Password; ToJsonUtilS(Password, each_Password); output["Password"] = each_Password;
                Json::Value each_Token; ToJsonUtilS(Token, each_Token); output["Token"] = each_Token;
                return output;
            }
        };

        struct ResetPasswordResult : public PlayFabResultCommon
        {

            ResetPasswordResult() :
                PlayFabResultCommon()
            {}

            ResetPasswordResult(const ResetPasswordResult&) :
                PlayFabResultCommon()
            {}

            ~ResetPasswordResult() = default;

            void FromJson(const Json::Value&) override
            {
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                return output;
            }
        };

        struct ResetUserStatisticsRequest : public PlayFabRequestCommon
        {
            std::map<std::string, std::string> CustomTags;
            std::string PlayFabId;

            ResetUserStatisticsRequest() :
                PlayFabRequestCommon(),
                CustomTags(),
                PlayFabId()
            {}

            ResetUserStatisticsRequest(const ResetUserStatisticsRequest& src) :
                PlayFabRequestCommon(),
                CustomTags(src.CustomTags),
                PlayFabId(src.PlayFabId)
            {}

            ~ResetUserStatisticsRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["CustomTags"], CustomTags);
                FromJsonUtilS(input["PlayFabId"], PlayFabId);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_CustomTags; ToJsonUtilS(CustomTags, each_CustomTags); output["CustomTags"] = each_CustomTags;
                Json::Value each_PlayFabId; ToJsonUtilS(PlayFabId, each_PlayFabId); output["PlayFabId"] = each_PlayFabId;
                return output;
            }
        };

        struct ResetUserStatisticsResult : public PlayFabResultCommon
        {

            ResetUserStatisticsResult() :
                PlayFabResultCommon()
            {}

            ResetUserStatisticsResult(const ResetUserStatisticsResult&) :
                PlayFabResultCommon()
            {}

            ~ResetUserStatisticsResult() = default;

            void FromJson(const Json::Value&) override
            {
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                return output;
            }
        };

        struct ResolvePurchaseDisputeRequest : public PlayFabRequestCommon
        {
            std::string OrderId;
            ResolutionOutcome Outcome;
            std::string PlayFabId;
            std::string Reason;

            ResolvePurchaseDisputeRequest() :
                PlayFabRequestCommon(),
                OrderId(),
                Outcome(),
                PlayFabId(),
                Reason()
            {}

            ResolvePurchaseDisputeRequest(const ResolvePurchaseDisputeRequest& src) :
                PlayFabRequestCommon(),
                OrderId(src.OrderId),
                Outcome(src.Outcome),
                PlayFabId(src.PlayFabId),
                Reason(src.Reason)
            {}

            ~ResolvePurchaseDisputeRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["OrderId"], OrderId);
                FromJsonEnum(input["Outcome"], Outcome);
                FromJsonUtilS(input["PlayFabId"], PlayFabId);
                FromJsonUtilS(input["Reason"], Reason);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_OrderId; ToJsonUtilS(OrderId, each_OrderId); output["OrderId"] = each_OrderId;
                Json::Value each_Outcome; ToJsonEnum(Outcome, each_Outcome); output["Outcome"] = each_Outcome;
                Json::Value each_PlayFabId; ToJsonUtilS(PlayFabId, each_PlayFabId); output["PlayFabId"] = each_PlayFabId;
                Json::Value each_Reason; ToJsonUtilS(Reason, each_Reason); output["Reason"] = each_Reason;
                return output;
            }
        };

        struct ResolvePurchaseDisputeResponse : public PlayFabResultCommon
        {
            std::string PurchaseStatus;

            ResolvePurchaseDisputeResponse() :
                PlayFabResultCommon(),
                PurchaseStatus()
            {}

            ResolvePurchaseDisputeResponse(const ResolvePurchaseDisputeResponse& src) :
                PlayFabResultCommon(),
                PurchaseStatus(src.PurchaseStatus)
            {}

            ~ResolvePurchaseDisputeResponse() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["PurchaseStatus"], PurchaseStatus);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_PurchaseStatus; ToJsonUtilS(PurchaseStatus, each_PurchaseStatus); output["PurchaseStatus"] = each_PurchaseStatus;
                return output;
            }
        };

        struct RevokeAllBansForUserRequest : public PlayFabRequestCommon
        {
            std::string PlayFabId;

            RevokeAllBansForUserRequest() :
                PlayFabRequestCommon(),
                PlayFabId()
            {}

            RevokeAllBansForUserRequest(const RevokeAllBansForUserRequest& src) :
                PlayFabRequestCommon(),
                PlayFabId(src.PlayFabId)
            {}

            ~RevokeAllBansForUserRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["PlayFabId"], PlayFabId);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_PlayFabId; ToJsonUtilS(PlayFabId, each_PlayFabId); output["PlayFabId"] = each_PlayFabId;
                return output;
            }
        };

        struct RevokeAllBansForUserResult : public PlayFabResultCommon
        {
            std::list<BanInfo> BanData;

            RevokeAllBansForUserResult() :
                PlayFabResultCommon(),
                BanData()
            {}

            RevokeAllBansForUserResult(const RevokeAllBansForUserResult& src) :
                PlayFabResultCommon(),
                BanData(src.BanData)
            {}

            ~RevokeAllBansForUserResult() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilO(input["BanData"], BanData);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_BanData; ToJsonUtilO(BanData, each_BanData); output["BanData"] = each_BanData;
                return output;
            }
        };

        struct RevokeBansRequest : public PlayFabRequestCommon
        {
            std::list<std::string> BanIds;

            RevokeBansRequest() :
                PlayFabRequestCommon(),
                BanIds()
            {}

            RevokeBansRequest(const RevokeBansRequest& src) :
                PlayFabRequestCommon(),
                BanIds(src.BanIds)
            {}

            ~RevokeBansRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["BanIds"], BanIds);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_BanIds; ToJsonUtilS(BanIds, each_BanIds); output["BanIds"] = each_BanIds;
                return output;
            }
        };

        struct RevokeBansResult : public PlayFabResultCommon
        {
            std::list<BanInfo> BanData;

            RevokeBansResult() :
                PlayFabResultCommon(),
                BanData()
            {}

            RevokeBansResult(const RevokeBansResult& src) :
                PlayFabResultCommon(),
                BanData(src.BanData)
            {}

            ~RevokeBansResult() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilO(input["BanData"], BanData);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_BanData; ToJsonUtilO(BanData, each_BanData); output["BanData"] = each_BanData;
                return output;
            }
        };

        struct RevokeInventoryItem : public PlayFabBaseModel
        {
            std::string CharacterId;
            std::string ItemInstanceId;
            std::string PlayFabId;

            RevokeInventoryItem() :
                PlayFabBaseModel(),
                CharacterId(),
                ItemInstanceId(),
                PlayFabId()
            {}

            RevokeInventoryItem(const RevokeInventoryItem& src) :
                PlayFabBaseModel(),
                CharacterId(src.CharacterId),
                ItemInstanceId(src.ItemInstanceId),
                PlayFabId(src.PlayFabId)
            {}

            ~RevokeInventoryItem() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["CharacterId"], CharacterId);
                FromJsonUtilS(input["ItemInstanceId"], ItemInstanceId);
                FromJsonUtilS(input["PlayFabId"], PlayFabId);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_CharacterId; ToJsonUtilS(CharacterId, each_CharacterId); output["CharacterId"] = each_CharacterId;
                Json::Value each_ItemInstanceId; ToJsonUtilS(ItemInstanceId, each_ItemInstanceId); output["ItemInstanceId"] = each_ItemInstanceId;
                Json::Value each_PlayFabId; ToJsonUtilS(PlayFabId, each_PlayFabId); output["PlayFabId"] = each_PlayFabId;
                return output;
            }
        };

        struct RevokeInventoryItemRequest : public PlayFabRequestCommon
        {
            std::string CharacterId;
            std::string ItemInstanceId;
            std::string PlayFabId;

            RevokeInventoryItemRequest() :
                PlayFabRequestCommon(),
                CharacterId(),
                ItemInstanceId(),
                PlayFabId()
            {}

            RevokeInventoryItemRequest(const RevokeInventoryItemRequest& src) :
                PlayFabRequestCommon(),
                CharacterId(src.CharacterId),
                ItemInstanceId(src.ItemInstanceId),
                PlayFabId(src.PlayFabId)
            {}

            ~RevokeInventoryItemRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["CharacterId"], CharacterId);
                FromJsonUtilS(input["ItemInstanceId"], ItemInstanceId);
                FromJsonUtilS(input["PlayFabId"], PlayFabId);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_CharacterId; ToJsonUtilS(CharacterId, each_CharacterId); output["CharacterId"] = each_CharacterId;
                Json::Value each_ItemInstanceId; ToJsonUtilS(ItemInstanceId, each_ItemInstanceId); output["ItemInstanceId"] = each_ItemInstanceId;
                Json::Value each_PlayFabId; ToJsonUtilS(PlayFabId, each_PlayFabId); output["PlayFabId"] = each_PlayFabId;
                return output;
            }
        };

        struct RevokeInventoryItemsRequest : public PlayFabRequestCommon
        {
            std::list<RevokeInventoryItem> Items;

            RevokeInventoryItemsRequest() :
                PlayFabRequestCommon(),
                Items()
            {}

            RevokeInventoryItemsRequest(const RevokeInventoryItemsRequest& src) :
                PlayFabRequestCommon(),
                Items(src.Items)
            {}

            ~RevokeInventoryItemsRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilO(input["Items"], Items);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_Items; ToJsonUtilO(Items, each_Items); output["Items"] = each_Items;
                return output;
            }
        };

        struct RevokeItemError : public PlayFabBaseModel
        {
            Boxed<GenericErrorCodes> Error;
            Boxed<RevokeInventoryItem> Item;

            RevokeItemError() :
                PlayFabBaseModel(),
                Error(),
                Item()
            {}

            RevokeItemError(const RevokeItemError& src) :
                PlayFabBaseModel(),
                Error(src.Error),
                Item(src.Item)
            {}

            ~RevokeItemError() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilE(input["Error"], Error);
                FromJsonUtilO(input["Item"], Item);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_Error; ToJsonUtilE(Error, each_Error); output["Error"] = each_Error;
                Json::Value each_Item; ToJsonUtilO(Item, each_Item); output["Item"] = each_Item;
                return output;
            }
        };

        struct RevokeInventoryItemsResult : public PlayFabResultCommon
        {
            std::list<RevokeItemError> Errors;

            RevokeInventoryItemsResult() :
                PlayFabResultCommon(),
                Errors()
            {}

            RevokeInventoryItemsResult(const RevokeInventoryItemsResult& src) :
                PlayFabResultCommon(),
                Errors(src.Errors)
            {}

            ~RevokeInventoryItemsResult() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilO(input["Errors"], Errors);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_Errors; ToJsonUtilO(Errors, each_Errors); output["Errors"] = each_Errors;
                return output;
            }
        };

        struct RevokeInventoryResult : public PlayFabResultCommon
        {

            RevokeInventoryResult() :
                PlayFabResultCommon()
            {}

            RevokeInventoryResult(const RevokeInventoryResult&) :
                PlayFabResultCommon()
            {}

            ~RevokeInventoryResult() = default;

            void FromJson(const Json::Value&) override
            {
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                return output;
            }
        };

        struct RunTaskRequest : public PlayFabRequestCommon
        {
            std::map<std::string, std::string> CustomTags;
            Boxed<NameIdentifier> Identifier;

            RunTaskRequest() :
                PlayFabRequestCommon(),
                CustomTags(),
                Identifier()
            {}

            RunTaskRequest(const RunTaskRequest& src) :
                PlayFabRequestCommon(),
                CustomTags(src.CustomTags),
                Identifier(src.Identifier)
            {}

            ~RunTaskRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["CustomTags"], CustomTags);
                FromJsonUtilO(input["Identifier"], Identifier);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_CustomTags; ToJsonUtilS(CustomTags, each_CustomTags); output["CustomTags"] = each_CustomTags;
                Json::Value each_Identifier; ToJsonUtilO(Identifier, each_Identifier); output["Identifier"] = each_Identifier;
                return output;
            }
        };

        struct RunTaskResult : public PlayFabResultCommon
        {
            std::string TaskInstanceId;

            RunTaskResult() :
                PlayFabResultCommon(),
                TaskInstanceId()
            {}

            RunTaskResult(const RunTaskResult& src) :
                PlayFabResultCommon(),
                TaskInstanceId(src.TaskInstanceId)
            {}

            ~RunTaskResult() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["TaskInstanceId"], TaskInstanceId);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_TaskInstanceId; ToJsonUtilS(TaskInstanceId, each_TaskInstanceId); output["TaskInstanceId"] = each_TaskInstanceId;
                return output;
            }
        };

        struct SendAccountRecoveryEmailRequest : public PlayFabRequestCommon
        {
            std::map<std::string, std::string> CustomTags;
            std::string Email;
            std::string EmailTemplateId;

            SendAccountRecoveryEmailRequest() :
                PlayFabRequestCommon(),
                CustomTags(),
                Email(),
                EmailTemplateId()
            {}

            SendAccountRecoveryEmailRequest(const SendAccountRecoveryEmailRequest& src) :
                PlayFabRequestCommon(),
                CustomTags(src.CustomTags),
                Email(src.Email),
                EmailTemplateId(src.EmailTemplateId)
            {}

            ~SendAccountRecoveryEmailRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["CustomTags"], CustomTags);
                FromJsonUtilS(input["Email"], Email);
                FromJsonUtilS(input["EmailTemplateId"], EmailTemplateId);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_CustomTags; ToJsonUtilS(CustomTags, each_CustomTags); output["CustomTags"] = each_CustomTags;
                Json::Value each_Email; ToJsonUtilS(Email, each_Email); output["Email"] = each_Email;
                Json::Value each_EmailTemplateId; ToJsonUtilS(EmailTemplateId, each_EmailTemplateId); output["EmailTemplateId"] = each_EmailTemplateId;
                return output;
            }
        };

        struct SendAccountRecoveryEmailResult : public PlayFabResultCommon
        {

            SendAccountRecoveryEmailResult() :
                PlayFabResultCommon()
            {}

            SendAccountRecoveryEmailResult(const SendAccountRecoveryEmailResult&) :
                PlayFabResultCommon()
            {}

            ~SendAccountRecoveryEmailResult() = default;

            void FromJson(const Json::Value&) override
            {
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                return output;
            }
        };

        struct SetPlayerSecretRequest : public PlayFabRequestCommon
        {
            std::string PlayerSecret;
            std::string PlayFabId;

            SetPlayerSecretRequest() :
                PlayFabRequestCommon(),
                PlayerSecret(),
                PlayFabId()
            {}

            SetPlayerSecretRequest(const SetPlayerSecretRequest& src) :
                PlayFabRequestCommon(),
                PlayerSecret(src.PlayerSecret),
                PlayFabId(src.PlayFabId)
            {}

            ~SetPlayerSecretRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["PlayerSecret"], PlayerSecret);
                FromJsonUtilS(input["PlayFabId"], PlayFabId);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_PlayerSecret; ToJsonUtilS(PlayerSecret, each_PlayerSecret); output["PlayerSecret"] = each_PlayerSecret;
                Json::Value each_PlayFabId; ToJsonUtilS(PlayFabId, each_PlayFabId); output["PlayFabId"] = each_PlayFabId;
                return output;
            }
        };

        struct SetPlayerSecretResult : public PlayFabResultCommon
        {

            SetPlayerSecretResult() :
                PlayFabResultCommon()
            {}

            SetPlayerSecretResult(const SetPlayerSecretResult&) :
                PlayFabResultCommon()
            {}

            ~SetPlayerSecretResult() = default;

            void FromJson(const Json::Value&) override
            {
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                return output;
            }
        };

        struct SetPublishedRevisionRequest : public PlayFabRequestCommon
        {
            std::map<std::string, std::string> CustomTags;
            Int32 Revision;
            Int32 Version;

            SetPublishedRevisionRequest() :
                PlayFabRequestCommon(),
                CustomTags(),
                Revision(),
                Version()
            {}

            SetPublishedRevisionRequest(const SetPublishedRevisionRequest& src) :
                PlayFabRequestCommon(),
                CustomTags(src.CustomTags),
                Revision(src.Revision),
                Version(src.Version)
            {}

            ~SetPublishedRevisionRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["CustomTags"], CustomTags);
                FromJsonUtilP(input["Revision"], Revision);
                FromJsonUtilP(input["Version"], Version);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_CustomTags; ToJsonUtilS(CustomTags, each_CustomTags); output["CustomTags"] = each_CustomTags;
                Json::Value each_Revision; ToJsonUtilP(Revision, each_Revision); output["Revision"] = each_Revision;
                Json::Value each_Version; ToJsonUtilP(Version, each_Version); output["Version"] = each_Version;
                return output;
            }
        };

        struct SetPublishedRevisionResult : public PlayFabResultCommon
        {

            SetPublishedRevisionResult() :
                PlayFabResultCommon()
            {}

            SetPublishedRevisionResult(const SetPublishedRevisionResult&) :
                PlayFabResultCommon()
            {}

            ~SetPublishedRevisionResult() = default;

            void FromJson(const Json::Value&) override
            {
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                return output;
            }
        };

        struct SetPublisherDataRequest : public PlayFabRequestCommon
        {
            std::string Key;
            std::string Value;

            SetPublisherDataRequest() :
                PlayFabRequestCommon(),
                Key(),
                Value()
            {}

            SetPublisherDataRequest(const SetPublisherDataRequest& src) :
                PlayFabRequestCommon(),
                Key(src.Key),
                Value(src.Value)
            {}

            ~SetPublisherDataRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["Key"], Key);
                FromJsonUtilS(input["Value"], Value);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_Key; ToJsonUtilS(Key, each_Key); output["Key"] = each_Key;
                Json::Value each_Value; ToJsonUtilS(Value, each_Value); output["Value"] = each_Value;
                return output;
            }
        };

        struct SetPublisherDataResult : public PlayFabResultCommon
        {

            SetPublisherDataResult() :
                PlayFabResultCommon()
            {}

            SetPublisherDataResult(const SetPublisherDataResult&) :
                PlayFabResultCommon()
            {}

            ~SetPublisherDataResult() = default;

            void FromJson(const Json::Value&) override
            {
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                return output;
            }
        };

        struct TitleDataKeyValue : public PlayFabBaseModel
        {
            std::string Key;
            std::string Value;

            TitleDataKeyValue() :
                PlayFabBaseModel(),
                Key(),
                Value()
            {}

            TitleDataKeyValue(const TitleDataKeyValue& src) :
                PlayFabBaseModel(),
                Key(src.Key),
                Value(src.Value)
            {}

            ~TitleDataKeyValue() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["Key"], Key);
                FromJsonUtilS(input["Value"], Value);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_Key; ToJsonUtilS(Key, each_Key); output["Key"] = each_Key;
                Json::Value each_Value; ToJsonUtilS(Value, each_Value); output["Value"] = each_Value;
                return output;
            }
        };

        struct SetTitleDataAndOverridesRequest : public PlayFabRequestCommon
        {
            std::list<TitleDataKeyValue> KeyValues;
            std::string OverrideLabel;

            SetTitleDataAndOverridesRequest() :
                PlayFabRequestCommon(),
                KeyValues(),
                OverrideLabel()
            {}

            SetTitleDataAndOverridesRequest(const SetTitleDataAndOverridesRequest& src) :
                PlayFabRequestCommon(),
                KeyValues(src.KeyValues),
                OverrideLabel(src.OverrideLabel)
            {}

            ~SetTitleDataAndOverridesRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilO(input["KeyValues"], KeyValues);
                FromJsonUtilS(input["OverrideLabel"], OverrideLabel);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_KeyValues; ToJsonUtilO(KeyValues, each_KeyValues); output["KeyValues"] = each_KeyValues;
                Json::Value each_OverrideLabel; ToJsonUtilS(OverrideLabel, each_OverrideLabel); output["OverrideLabel"] = each_OverrideLabel;
                return output;
            }
        };

        struct SetTitleDataAndOverridesResult : public PlayFabResultCommon
        {

            SetTitleDataAndOverridesResult() :
                PlayFabResultCommon()
            {}

            SetTitleDataAndOverridesResult(const SetTitleDataAndOverridesResult&) :
                PlayFabResultCommon()
            {}

            ~SetTitleDataAndOverridesResult() = default;

            void FromJson(const Json::Value&) override
            {
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                return output;
            }
        };

        struct SetTitleDataRequest : public PlayFabRequestCommon
        {
            std::string Key;
            std::string Value;

            SetTitleDataRequest() :
                PlayFabRequestCommon(),
                Key(),
                Value()
            {}

            SetTitleDataRequest(const SetTitleDataRequest& src) :
                PlayFabRequestCommon(),
                Key(src.Key),
                Value(src.Value)
            {}

            ~SetTitleDataRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["Key"], Key);
                FromJsonUtilS(input["Value"], Value);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_Key; ToJsonUtilS(Key, each_Key); output["Key"] = each_Key;
                Json::Value each_Value; ToJsonUtilS(Value, each_Value); output["Value"] = each_Value;
                return output;
            }
        };

        struct SetTitleDataResult : public PlayFabResultCommon
        {

            SetTitleDataResult() :
                PlayFabResultCommon()
            {}

            SetTitleDataResult(const SetTitleDataResult&) :
                PlayFabResultCommon()
            {}

            ~SetTitleDataResult() = default;

            void FromJson(const Json::Value&) override
            {
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                return output;
            }
        };

        struct SetupPushNotificationRequest : public PlayFabRequestCommon
        {
            std::string Credential;
            std::string Key;
            std::string Name;
            bool OverwriteOldARN;
            PushSetupPlatform Platform;

            SetupPushNotificationRequest() :
                PlayFabRequestCommon(),
                Credential(),
                Key(),
                Name(),
                OverwriteOldARN(),
                Platform()
            {}

            SetupPushNotificationRequest(const SetupPushNotificationRequest& src) :
                PlayFabRequestCommon(),
                Credential(src.Credential),
                Key(src.Key),
                Name(src.Name),
                OverwriteOldARN(src.OverwriteOldARN),
                Platform(src.Platform)
            {}

            ~SetupPushNotificationRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["Credential"], Credential);
                FromJsonUtilS(input["Key"], Key);
                FromJsonUtilS(input["Name"], Name);
                FromJsonUtilP(input["OverwriteOldARN"], OverwriteOldARN);
                FromJsonEnum(input["Platform"], Platform);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_Credential; ToJsonUtilS(Credential, each_Credential); output["Credential"] = each_Credential;
                Json::Value each_Key; ToJsonUtilS(Key, each_Key); output["Key"] = each_Key;
                Json::Value each_Name; ToJsonUtilS(Name, each_Name); output["Name"] = each_Name;
                Json::Value each_OverwriteOldARN; ToJsonUtilP(OverwriteOldARN, each_OverwriteOldARN); output["OverwriteOldARN"] = each_OverwriteOldARN;
                Json::Value each_Platform; ToJsonEnum(Platform, each_Platform); output["Platform"] = each_Platform;
                return output;
            }
        };

        struct SetupPushNotificationResult : public PlayFabResultCommon
        {
            std::string ARN;

            SetupPushNotificationResult() :
                PlayFabResultCommon(),
                ARN()
            {}

            SetupPushNotificationResult(const SetupPushNotificationResult& src) :
                PlayFabResultCommon(),
                ARN(src.ARN)
            {}

            ~SetupPushNotificationResult() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["ARN"], ARN);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_ARN; ToJsonUtilS(ARN, each_ARN); output["ARN"] = each_ARN;
                return output;
            }
        };

        struct SubtractUserVirtualCurrencyRequest : public PlayFabRequestCommon
        {
            Int32 Amount;
            std::map<std::string, std::string> CustomTags;
            std::string PlayFabId;
            std::string VirtualCurrency;

            SubtractUserVirtualCurrencyRequest() :
                PlayFabRequestCommon(),
                Amount(),
                CustomTags(),
                PlayFabId(),
                VirtualCurrency()
            {}

            SubtractUserVirtualCurrencyRequest(const SubtractUserVirtualCurrencyRequest& src) :
                PlayFabRequestCommon(),
                Amount(src.Amount),
                CustomTags(src.CustomTags),
                PlayFabId(src.PlayFabId),
                VirtualCurrency(src.VirtualCurrency)
            {}

            ~SubtractUserVirtualCurrencyRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilP(input["Amount"], Amount);
                FromJsonUtilS(input["CustomTags"], CustomTags);
                FromJsonUtilS(input["PlayFabId"], PlayFabId);
                FromJsonUtilS(input["VirtualCurrency"], VirtualCurrency);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_Amount; ToJsonUtilP(Amount, each_Amount); output["Amount"] = each_Amount;
                Json::Value each_CustomTags; ToJsonUtilS(CustomTags, each_CustomTags); output["CustomTags"] = each_CustomTags;
                Json::Value each_PlayFabId; ToJsonUtilS(PlayFabId, each_PlayFabId); output["PlayFabId"] = each_PlayFabId;
                Json::Value each_VirtualCurrency; ToJsonUtilS(VirtualCurrency, each_VirtualCurrency); output["VirtualCurrency"] = each_VirtualCurrency;
                return output;
            }
        };

        struct UpdateBanRequest : public PlayFabRequestCommon
        {
            Boxed<bool> Active;
            std::string BanId;
            Boxed<time_t> Expires;
            std::string IPAddress;
            std::string MACAddress;
            Boxed<bool> Permanent;
            std::string Reason;

            UpdateBanRequest() :
                PlayFabRequestCommon(),
                Active(),
                BanId(),
                Expires(),
                IPAddress(),
                MACAddress(),
                Permanent(),
                Reason()
            {}

            UpdateBanRequest(const UpdateBanRequest& src) :
                PlayFabRequestCommon(),
                Active(src.Active),
                BanId(src.BanId),
                Expires(src.Expires),
                IPAddress(src.IPAddress),
                MACAddress(src.MACAddress),
                Permanent(src.Permanent),
                Reason(src.Reason)
            {}

            ~UpdateBanRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilP(input["Active"], Active);
                FromJsonUtilS(input["BanId"], BanId);
                FromJsonUtilT(input["Expires"], Expires);
                FromJsonUtilS(input["IPAddress"], IPAddress);
                FromJsonUtilS(input["MACAddress"], MACAddress);
                FromJsonUtilP(input["Permanent"], Permanent);
                FromJsonUtilS(input["Reason"], Reason);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_Active; ToJsonUtilP(Active, each_Active); output["Active"] = each_Active;
                Json::Value each_BanId; ToJsonUtilS(BanId, each_BanId); output["BanId"] = each_BanId;
                Json::Value each_Expires; ToJsonUtilT(Expires, each_Expires); output["Expires"] = each_Expires;
                Json::Value each_IPAddress; ToJsonUtilS(IPAddress, each_IPAddress); output["IPAddress"] = each_IPAddress;
                Json::Value each_MACAddress; ToJsonUtilS(MACAddress, each_MACAddress); output["MACAddress"] = each_MACAddress;
                Json::Value each_Permanent; ToJsonUtilP(Permanent, each_Permanent); output["Permanent"] = each_Permanent;
                Json::Value each_Reason; ToJsonUtilS(Reason, each_Reason); output["Reason"] = each_Reason;
                return output;
            }
        };

        struct UpdateBansRequest : public PlayFabRequestCommon
        {
            std::list<UpdateBanRequest> Bans;

            UpdateBansRequest() :
                PlayFabRequestCommon(),
                Bans()
            {}

            UpdateBansRequest(const UpdateBansRequest& src) :
                PlayFabRequestCommon(),
                Bans(src.Bans)
            {}

            ~UpdateBansRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilO(input["Bans"], Bans);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_Bans; ToJsonUtilO(Bans, each_Bans); output["Bans"] = each_Bans;
                return output;
            }
        };

        struct UpdateBansResult : public PlayFabResultCommon
        {
            std::list<BanInfo> BanData;

            UpdateBansResult() :
                PlayFabResultCommon(),
                BanData()
            {}

            UpdateBansResult(const UpdateBansResult& src) :
                PlayFabResultCommon(),
                BanData(src.BanData)
            {}

            ~UpdateBansResult() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilO(input["BanData"], BanData);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_BanData; ToJsonUtilO(BanData, each_BanData); output["BanData"] = each_BanData;
                return output;
            }
        };

        struct UpdateCatalogItemsRequest : public PlayFabRequestCommon
        {
            std::list<CatalogItem> Catalog;
            std::string CatalogVersion;
            std::map<std::string, std::string> CustomTags;
            Boxed<bool> SetAsDefaultCatalog;

            UpdateCatalogItemsRequest() :
                PlayFabRequestCommon(),
                Catalog(),
                CatalogVersion(),
                CustomTags(),
                SetAsDefaultCatalog()
            {}

            UpdateCatalogItemsRequest(const UpdateCatalogItemsRequest& src) :
                PlayFabRequestCommon(),
                Catalog(src.Catalog),
                CatalogVersion(src.CatalogVersion),
                CustomTags(src.CustomTags),
                SetAsDefaultCatalog(src.SetAsDefaultCatalog)
            {}

            ~UpdateCatalogItemsRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilO(input["Catalog"], Catalog);
                FromJsonUtilS(input["CatalogVersion"], CatalogVersion);
                FromJsonUtilS(input["CustomTags"], CustomTags);
                FromJsonUtilP(input["SetAsDefaultCatalog"], SetAsDefaultCatalog);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_Catalog; ToJsonUtilO(Catalog, each_Catalog); output["Catalog"] = each_Catalog;
                Json::Value each_CatalogVersion; ToJsonUtilS(CatalogVersion, each_CatalogVersion); output["CatalogVersion"] = each_CatalogVersion;
                Json::Value each_CustomTags; ToJsonUtilS(CustomTags, each_CustomTags); output["CustomTags"] = each_CustomTags;
                Json::Value each_SetAsDefaultCatalog; ToJsonUtilP(SetAsDefaultCatalog, each_SetAsDefaultCatalog); output["SetAsDefaultCatalog"] = each_SetAsDefaultCatalog;
                return output;
            }
        };

        struct UpdateCatalogItemsResult : public PlayFabResultCommon
        {

            UpdateCatalogItemsResult() :
                PlayFabResultCommon()
            {}

            UpdateCatalogItemsResult(const UpdateCatalogItemsResult&) :
                PlayFabResultCommon()
            {}

            ~UpdateCatalogItemsResult() = default;

            void FromJson(const Json::Value&) override
            {
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                return output;
            }
        };

        struct UpdateCloudScriptRequest : public PlayFabRequestCommon
        {
            std::map<std::string, std::string> CustomTags;
            std::string DeveloperPlayFabId;
            std::list<CloudScriptFile> Files;
            bool Publish;

            UpdateCloudScriptRequest() :
                PlayFabRequestCommon(),
                CustomTags(),
                DeveloperPlayFabId(),
                Files(),
                Publish()
            {}

            UpdateCloudScriptRequest(const UpdateCloudScriptRequest& src) :
                PlayFabRequestCommon(),
                CustomTags(src.CustomTags),
                DeveloperPlayFabId(src.DeveloperPlayFabId),
                Files(src.Files),
                Publish(src.Publish)
            {}

            ~UpdateCloudScriptRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["CustomTags"], CustomTags);
                FromJsonUtilS(input["DeveloperPlayFabId"], DeveloperPlayFabId);
                FromJsonUtilO(input["Files"], Files);
                FromJsonUtilP(input["Publish"], Publish);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_CustomTags; ToJsonUtilS(CustomTags, each_CustomTags); output["CustomTags"] = each_CustomTags;
                Json::Value each_DeveloperPlayFabId; ToJsonUtilS(DeveloperPlayFabId, each_DeveloperPlayFabId); output["DeveloperPlayFabId"] = each_DeveloperPlayFabId;
                Json::Value each_Files; ToJsonUtilO(Files, each_Files); output["Files"] = each_Files;
                Json::Value each_Publish; ToJsonUtilP(Publish, each_Publish); output["Publish"] = each_Publish;
                return output;
            }
        };

        struct UpdateCloudScriptResult : public PlayFabResultCommon
        {
            Int32 Revision;
            Int32 Version;

            UpdateCloudScriptResult() :
                PlayFabResultCommon(),
                Revision(),
                Version()
            {}

            UpdateCloudScriptResult(const UpdateCloudScriptResult& src) :
                PlayFabResultCommon(),
                Revision(src.Revision),
                Version(src.Version)
            {}

            ~UpdateCloudScriptResult() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilP(input["Revision"], Revision);
                FromJsonUtilP(input["Version"], Version);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_Revision; ToJsonUtilP(Revision, each_Revision); output["Revision"] = each_Revision;
                Json::Value each_Version; ToJsonUtilP(Version, each_Version); output["Version"] = each_Version;
                return output;
            }
        };

        struct UpdateOpenIdConnectionRequest : public PlayFabRequestCommon
        {
            std::string ClientId;
            std::string ClientSecret;
            std::string ConnectionId;
            std::string IssuerDiscoveryUrl;
            Boxed<OpenIdIssuerInformation> IssuerInformation;

            UpdateOpenIdConnectionRequest() :
                PlayFabRequestCommon(),
                ClientId(),
                ClientSecret(),
                ConnectionId(),
                IssuerDiscoveryUrl(),
                IssuerInformation()
            {}

            UpdateOpenIdConnectionRequest(const UpdateOpenIdConnectionRequest& src) :
                PlayFabRequestCommon(),
                ClientId(src.ClientId),
                ClientSecret(src.ClientSecret),
                ConnectionId(src.ConnectionId),
                IssuerDiscoveryUrl(src.IssuerDiscoveryUrl),
                IssuerInformation(src.IssuerInformation)
            {}

            ~UpdateOpenIdConnectionRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["ClientId"], ClientId);
                FromJsonUtilS(input["ClientSecret"], ClientSecret);
                FromJsonUtilS(input["ConnectionId"], ConnectionId);
                FromJsonUtilS(input["IssuerDiscoveryUrl"], IssuerDiscoveryUrl);
                FromJsonUtilO(input["IssuerInformation"], IssuerInformation);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_ClientId; ToJsonUtilS(ClientId, each_ClientId); output["ClientId"] = each_ClientId;
                Json::Value each_ClientSecret; ToJsonUtilS(ClientSecret, each_ClientSecret); output["ClientSecret"] = each_ClientSecret;
                Json::Value each_ConnectionId; ToJsonUtilS(ConnectionId, each_ConnectionId); output["ConnectionId"] = each_ConnectionId;
                Json::Value each_IssuerDiscoveryUrl; ToJsonUtilS(IssuerDiscoveryUrl, each_IssuerDiscoveryUrl); output["IssuerDiscoveryUrl"] = each_IssuerDiscoveryUrl;
                Json::Value each_IssuerInformation; ToJsonUtilO(IssuerInformation, each_IssuerInformation); output["IssuerInformation"] = each_IssuerInformation;
                return output;
            }
        };

        struct UpdatePlayerSharedSecretRequest : public PlayFabRequestCommon
        {
            bool Disabled;
            std::string FriendlyName;
            std::string SecretKey;

            UpdatePlayerSharedSecretRequest() :
                PlayFabRequestCommon(),
                Disabled(),
                FriendlyName(),
                SecretKey()
            {}

            UpdatePlayerSharedSecretRequest(const UpdatePlayerSharedSecretRequest& src) :
                PlayFabRequestCommon(),
                Disabled(src.Disabled),
                FriendlyName(src.FriendlyName),
                SecretKey(src.SecretKey)
            {}

            ~UpdatePlayerSharedSecretRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilP(input["Disabled"], Disabled);
                FromJsonUtilS(input["FriendlyName"], FriendlyName);
                FromJsonUtilS(input["SecretKey"], SecretKey);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_Disabled; ToJsonUtilP(Disabled, each_Disabled); output["Disabled"] = each_Disabled;
                Json::Value each_FriendlyName; ToJsonUtilS(FriendlyName, each_FriendlyName); output["FriendlyName"] = each_FriendlyName;
                Json::Value each_SecretKey; ToJsonUtilS(SecretKey, each_SecretKey); output["SecretKey"] = each_SecretKey;
                return output;
            }
        };

        struct UpdatePlayerSharedSecretResult : public PlayFabResultCommon
        {

            UpdatePlayerSharedSecretResult() :
                PlayFabResultCommon()
            {}

            UpdatePlayerSharedSecretResult(const UpdatePlayerSharedSecretResult&) :
                PlayFabResultCommon()
            {}

            ~UpdatePlayerSharedSecretResult() = default;

            void FromJson(const Json::Value&) override
            {
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                return output;
            }
        };

        struct UpdatePlayerStatisticDefinitionRequest : public PlayFabRequestCommon
        {
            Boxed<StatisticAggregationMethod> AggregationMethod;
            std::string StatisticName;
            Boxed<StatisticResetIntervalOption> VersionChangeInterval;

            UpdatePlayerStatisticDefinitionRequest() :
                PlayFabRequestCommon(),
                AggregationMethod(),
                StatisticName(),
                VersionChangeInterval()
            {}

            UpdatePlayerStatisticDefinitionRequest(const UpdatePlayerStatisticDefinitionRequest& src) :
                PlayFabRequestCommon(),
                AggregationMethod(src.AggregationMethod),
                StatisticName(src.StatisticName),
                VersionChangeInterval(src.VersionChangeInterval)
            {}

            ~UpdatePlayerStatisticDefinitionRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilE(input["AggregationMethod"], AggregationMethod);
                FromJsonUtilS(input["StatisticName"], StatisticName);
                FromJsonUtilE(input["VersionChangeInterval"], VersionChangeInterval);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_AggregationMethod; ToJsonUtilE(AggregationMethod, each_AggregationMethod); output["AggregationMethod"] = each_AggregationMethod;
                Json::Value each_StatisticName; ToJsonUtilS(StatisticName, each_StatisticName); output["StatisticName"] = each_StatisticName;
                Json::Value each_VersionChangeInterval; ToJsonUtilE(VersionChangeInterval, each_VersionChangeInterval); output["VersionChangeInterval"] = each_VersionChangeInterval;
                return output;
            }
        };

        struct UpdatePlayerStatisticDefinitionResult : public PlayFabResultCommon
        {
            Boxed<PlayerStatisticDefinition> Statistic;

            UpdatePlayerStatisticDefinitionResult() :
                PlayFabResultCommon(),
                Statistic()
            {}

            UpdatePlayerStatisticDefinitionResult(const UpdatePlayerStatisticDefinitionResult& src) :
                PlayFabResultCommon(),
                Statistic(src.Statistic)
            {}

            ~UpdatePlayerStatisticDefinitionResult() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilO(input["Statistic"], Statistic);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_Statistic; ToJsonUtilO(Statistic, each_Statistic); output["Statistic"] = each_Statistic;
                return output;
            }
        };

        struct UpdatePolicyRequest : public PlayFabRequestCommon
        {
            bool OverwritePolicy;
            std::string PolicyName;
            std::list<PermissionStatement> Statements;

            UpdatePolicyRequest() :
                PlayFabRequestCommon(),
                OverwritePolicy(),
                PolicyName(),
                Statements()
            {}

            UpdatePolicyRequest(const UpdatePolicyRequest& src) :
                PlayFabRequestCommon(),
                OverwritePolicy(src.OverwritePolicy),
                PolicyName(src.PolicyName),
                Statements(src.Statements)
            {}

            ~UpdatePolicyRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilP(input["OverwritePolicy"], OverwritePolicy);
                FromJsonUtilS(input["PolicyName"], PolicyName);
                FromJsonUtilO(input["Statements"], Statements);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_OverwritePolicy; ToJsonUtilP(OverwritePolicy, each_OverwritePolicy); output["OverwritePolicy"] = each_OverwritePolicy;
                Json::Value each_PolicyName; ToJsonUtilS(PolicyName, each_PolicyName); output["PolicyName"] = each_PolicyName;
                Json::Value each_Statements; ToJsonUtilO(Statements, each_Statements); output["Statements"] = each_Statements;
                return output;
            }
        };

        struct UpdatePolicyResponse : public PlayFabResultCommon
        {
            std::string PolicyName;
            std::list<PermissionStatement> Statements;

            UpdatePolicyResponse() :
                PlayFabResultCommon(),
                PolicyName(),
                Statements()
            {}

            UpdatePolicyResponse(const UpdatePolicyResponse& src) :
                PlayFabResultCommon(),
                PolicyName(src.PolicyName),
                Statements(src.Statements)
            {}

            ~UpdatePolicyResponse() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["PolicyName"], PolicyName);
                FromJsonUtilO(input["Statements"], Statements);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_PolicyName; ToJsonUtilS(PolicyName, each_PolicyName); output["PolicyName"] = each_PolicyName;
                Json::Value each_Statements; ToJsonUtilO(Statements, each_Statements); output["Statements"] = each_Statements;
                return output;
            }
        };

        struct UpdateRandomResultTablesRequest : public PlayFabRequestCommon
        {
            std::string CatalogVersion;
            std::map<std::string, std::string> CustomTags;
            std::list<RandomResultTable> Tables;

            UpdateRandomResultTablesRequest() :
                PlayFabRequestCommon(),
                CatalogVersion(),
                CustomTags(),
                Tables()
            {}

            UpdateRandomResultTablesRequest(const UpdateRandomResultTablesRequest& src) :
                PlayFabRequestCommon(),
                CatalogVersion(src.CatalogVersion),
                CustomTags(src.CustomTags),
                Tables(src.Tables)
            {}

            ~UpdateRandomResultTablesRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["CatalogVersion"], CatalogVersion);
                FromJsonUtilS(input["CustomTags"], CustomTags);
                FromJsonUtilO(input["Tables"], Tables);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_CatalogVersion; ToJsonUtilS(CatalogVersion, each_CatalogVersion); output["CatalogVersion"] = each_CatalogVersion;
                Json::Value each_CustomTags; ToJsonUtilS(CustomTags, each_CustomTags); output["CustomTags"] = each_CustomTags;
                Json::Value each_Tables; ToJsonUtilO(Tables, each_Tables); output["Tables"] = each_Tables;
                return output;
            }
        };

        struct UpdateRandomResultTablesResult : public PlayFabResultCommon
        {

            UpdateRandomResultTablesResult() :
                PlayFabResultCommon()
            {}

            UpdateRandomResultTablesResult(const UpdateRandomResultTablesResult&) :
                PlayFabResultCommon()
            {}

            ~UpdateRandomResultTablesResult() = default;

            void FromJson(const Json::Value&) override
            {
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                return output;
            }
        };

        struct UpdateStoreItemsRequest : public PlayFabRequestCommon
        {
            std::string CatalogVersion;
            std::map<std::string, std::string> CustomTags;
            Boxed<StoreMarketingModel> MarketingData;
            std::list<StoreItem> Store;
            std::string StoreId;

            UpdateStoreItemsRequest() :
                PlayFabRequestCommon(),
                CatalogVersion(),
                CustomTags(),
                MarketingData(),
                Store(),
                StoreId()
            {}

            UpdateStoreItemsRequest(const UpdateStoreItemsRequest& src) :
                PlayFabRequestCommon(),
                CatalogVersion(src.CatalogVersion),
                CustomTags(src.CustomTags),
                MarketingData(src.MarketingData),
                Store(src.Store),
                StoreId(src.StoreId)
            {}

            ~UpdateStoreItemsRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["CatalogVersion"], CatalogVersion);
                FromJsonUtilS(input["CustomTags"], CustomTags);
                FromJsonUtilO(input["MarketingData"], MarketingData);
                FromJsonUtilO(input["Store"], Store);
                FromJsonUtilS(input["StoreId"], StoreId);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_CatalogVersion; ToJsonUtilS(CatalogVersion, each_CatalogVersion); output["CatalogVersion"] = each_CatalogVersion;
                Json::Value each_CustomTags; ToJsonUtilS(CustomTags, each_CustomTags); output["CustomTags"] = each_CustomTags;
                Json::Value each_MarketingData; ToJsonUtilO(MarketingData, each_MarketingData); output["MarketingData"] = each_MarketingData;
                Json::Value each_Store; ToJsonUtilO(Store, each_Store); output["Store"] = each_Store;
                Json::Value each_StoreId; ToJsonUtilS(StoreId, each_StoreId); output["StoreId"] = each_StoreId;
                return output;
            }
        };

        struct UpdateStoreItemsResult : public PlayFabResultCommon
        {

            UpdateStoreItemsResult() :
                PlayFabResultCommon()
            {}

            UpdateStoreItemsResult(const UpdateStoreItemsResult&) :
                PlayFabResultCommon()
            {}

            ~UpdateStoreItemsResult() = default;

            void FromJson(const Json::Value&) override
            {
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                return output;
            }
        };

        struct UpdateTaskRequest : public PlayFabRequestCommon
        {
            std::map<std::string, std::string> CustomTags;
            std::string Description;
            Boxed<NameIdentifier> Identifier;
            bool IsActive;
            std::string Name;
            Json::Value Parameter;
            std::string Schedule;
            ScheduledTaskType Type;

            UpdateTaskRequest() :
                PlayFabRequestCommon(),
                CustomTags(),
                Description(),
                Identifier(),
                IsActive(),
                Name(),
                Parameter(),
                Schedule(),
                Type()
            {}

            UpdateTaskRequest(const UpdateTaskRequest& src) :
                PlayFabRequestCommon(),
                CustomTags(src.CustomTags),
                Description(src.Description),
                Identifier(src.Identifier),
                IsActive(src.IsActive),
                Name(src.Name),
                Parameter(src.Parameter),
                Schedule(src.Schedule),
                Type(src.Type)
            {}

            ~UpdateTaskRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["CustomTags"], CustomTags);
                FromJsonUtilS(input["Description"], Description);
                FromJsonUtilO(input["Identifier"], Identifier);
                FromJsonUtilP(input["IsActive"], IsActive);
                FromJsonUtilS(input["Name"], Name);
                Parameter = input["Parameter"];
                FromJsonUtilS(input["Schedule"], Schedule);
                FromJsonEnum(input["Type"], Type);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_CustomTags; ToJsonUtilS(CustomTags, each_CustomTags); output["CustomTags"] = each_CustomTags;
                Json::Value each_Description; ToJsonUtilS(Description, each_Description); output["Description"] = each_Description;
                Json::Value each_Identifier; ToJsonUtilO(Identifier, each_Identifier); output["Identifier"] = each_Identifier;
                Json::Value each_IsActive; ToJsonUtilP(IsActive, each_IsActive); output["IsActive"] = each_IsActive;
                Json::Value each_Name; ToJsonUtilS(Name, each_Name); output["Name"] = each_Name;
                output["Parameter"] = Parameter;
                Json::Value each_Schedule; ToJsonUtilS(Schedule, each_Schedule); output["Schedule"] = each_Schedule;
                Json::Value each_Type; ToJsonEnum(Type, each_Type); output["Type"] = each_Type;
                return output;
            }
        };

        struct UpdateUserDataRequest : public PlayFabRequestCommon
        {
            std::map<std::string, std::string> CustomTags;
            std::map<std::string, std::string> Data;
            std::list<std::string> KeysToRemove;
            Boxed<UserDataPermission> Permission;
            std::string PlayFabId;

            UpdateUserDataRequest() :
                PlayFabRequestCommon(),
                CustomTags(),
                Data(),
                KeysToRemove(),
                Permission(),
                PlayFabId()
            {}

            UpdateUserDataRequest(const UpdateUserDataRequest& src) :
                PlayFabRequestCommon(),
                CustomTags(src.CustomTags),
                Data(src.Data),
                KeysToRemove(src.KeysToRemove),
                Permission(src.Permission),
                PlayFabId(src.PlayFabId)
            {}

            ~UpdateUserDataRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["CustomTags"], CustomTags);
                FromJsonUtilS(input["Data"], Data);
                FromJsonUtilS(input["KeysToRemove"], KeysToRemove);
                FromJsonUtilE(input["Permission"], Permission);
                FromJsonUtilS(input["PlayFabId"], PlayFabId);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_CustomTags; ToJsonUtilS(CustomTags, each_CustomTags); output["CustomTags"] = each_CustomTags;
                Json::Value each_Data; ToJsonUtilS(Data, each_Data); output["Data"] = each_Data;
                Json::Value each_KeysToRemove; ToJsonUtilS(KeysToRemove, each_KeysToRemove); output["KeysToRemove"] = each_KeysToRemove;
                Json::Value each_Permission; ToJsonUtilE(Permission, each_Permission); output["Permission"] = each_Permission;
                Json::Value each_PlayFabId; ToJsonUtilS(PlayFabId, each_PlayFabId); output["PlayFabId"] = each_PlayFabId;
                return output;
            }
        };

        struct UpdateUserDataResult : public PlayFabResultCommon
        {
            Uint32 DataVersion;

            UpdateUserDataResult() :
                PlayFabResultCommon(),
                DataVersion()
            {}

            UpdateUserDataResult(const UpdateUserDataResult& src) :
                PlayFabResultCommon(),
                DataVersion(src.DataVersion)
            {}

            ~UpdateUserDataResult() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilP(input["DataVersion"], DataVersion);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_DataVersion; ToJsonUtilP(DataVersion, each_DataVersion); output["DataVersion"] = each_DataVersion;
                return output;
            }
        };

        struct UpdateUserInternalDataRequest : public PlayFabRequestCommon
        {
            std::map<std::string, std::string> CustomTags;
            std::map<std::string, std::string> Data;
            std::list<std::string> KeysToRemove;
            std::string PlayFabId;

            UpdateUserInternalDataRequest() :
                PlayFabRequestCommon(),
                CustomTags(),
                Data(),
                KeysToRemove(),
                PlayFabId()
            {}

            UpdateUserInternalDataRequest(const UpdateUserInternalDataRequest& src) :
                PlayFabRequestCommon(),
                CustomTags(src.CustomTags),
                Data(src.Data),
                KeysToRemove(src.KeysToRemove),
                PlayFabId(src.PlayFabId)
            {}

            ~UpdateUserInternalDataRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["CustomTags"], CustomTags);
                FromJsonUtilS(input["Data"], Data);
                FromJsonUtilS(input["KeysToRemove"], KeysToRemove);
                FromJsonUtilS(input["PlayFabId"], PlayFabId);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_CustomTags; ToJsonUtilS(CustomTags, each_CustomTags); output["CustomTags"] = each_CustomTags;
                Json::Value each_Data; ToJsonUtilS(Data, each_Data); output["Data"] = each_Data;
                Json::Value each_KeysToRemove; ToJsonUtilS(KeysToRemove, each_KeysToRemove); output["KeysToRemove"] = each_KeysToRemove;
                Json::Value each_PlayFabId; ToJsonUtilS(PlayFabId, each_PlayFabId); output["PlayFabId"] = each_PlayFabId;
                return output;
            }
        };

        struct UpdateUserTitleDisplayNameRequest : public PlayFabRequestCommon
        {
            std::map<std::string, std::string> CustomTags;
            std::string DisplayName;
            std::string PlayFabId;

            UpdateUserTitleDisplayNameRequest() :
                PlayFabRequestCommon(),
                CustomTags(),
                DisplayName(),
                PlayFabId()
            {}

            UpdateUserTitleDisplayNameRequest(const UpdateUserTitleDisplayNameRequest& src) :
                PlayFabRequestCommon(),
                CustomTags(src.CustomTags),
                DisplayName(src.DisplayName),
                PlayFabId(src.PlayFabId)
            {}

            ~UpdateUserTitleDisplayNameRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["CustomTags"], CustomTags);
                FromJsonUtilS(input["DisplayName"], DisplayName);
                FromJsonUtilS(input["PlayFabId"], PlayFabId);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_CustomTags; ToJsonUtilS(CustomTags, each_CustomTags); output["CustomTags"] = each_CustomTags;
                Json::Value each_DisplayName; ToJsonUtilS(DisplayName, each_DisplayName); output["DisplayName"] = each_DisplayName;
                Json::Value each_PlayFabId; ToJsonUtilS(PlayFabId, each_PlayFabId); output["PlayFabId"] = each_PlayFabId;
                return output;
            }
        };

        struct UpdateUserTitleDisplayNameResult : public PlayFabResultCommon
        {
            std::string DisplayName;

            UpdateUserTitleDisplayNameResult() :
                PlayFabResultCommon(),
                DisplayName()
            {}

            UpdateUserTitleDisplayNameResult(const UpdateUserTitleDisplayNameResult& src) :
                PlayFabResultCommon(),
                DisplayName(src.DisplayName)
            {}

            ~UpdateUserTitleDisplayNameResult() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["DisplayName"], DisplayName);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_DisplayName; ToJsonUtilS(DisplayName, each_DisplayName); output["DisplayName"] = each_DisplayName;
                return output;
            }
        };

    }
}

#endif
