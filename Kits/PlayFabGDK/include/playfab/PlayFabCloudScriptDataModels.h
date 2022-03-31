#pragma once

#if !defined(DISABLE_PLAYFABENTITY_API)

#include <playfab/PlayFabBaseModel.h>
#include <playfab/PlayFabJsonHeaders.h>

namespace PlayFab
{
    namespace CloudScriptModels
    {
        // CloudScript Enums
        enum class CloudScriptRevisionOption
        {
            CloudScriptRevisionOptionLive,
            CloudScriptRevisionOptionLatest,
            CloudScriptRevisionOptionSpecific
        };

        inline void ToJsonEnum(const CloudScriptRevisionOption input, Json::Value& output)
        {
            if (input == CloudScriptRevisionOption::CloudScriptRevisionOptionLive)
            {
                output = Json::Value("Live");
                return;
            }
            if (input == CloudScriptRevisionOption::CloudScriptRevisionOptionLatest)
            {
                output = Json::Value("Latest");
                return;
            }
            if (input == CloudScriptRevisionOption::CloudScriptRevisionOptionSpecific)
            {
                output = Json::Value("Specific");
                return;
            }
        }
        inline void FromJsonEnum(const Json::Value& input, CloudScriptRevisionOption& output)
        {
            if (!input.isString())
            {
                return;
            }
            const std::string& inputStr = input.asString();
            if (inputStr == "Live")
            {
                output = CloudScriptRevisionOption::CloudScriptRevisionOptionLive;
                return;
            }
            if (inputStr == "Latest")
            {
                output = CloudScriptRevisionOption::CloudScriptRevisionOptionLatest;
                return;
            }
            if (inputStr == "Specific")
            {
                output = CloudScriptRevisionOption::CloudScriptRevisionOptionSpecific;
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

        enum class TriggerType
        {
            TriggerTypeHTTP,
            TriggerTypeQueue
        };

        inline void ToJsonEnum(const TriggerType input, Json::Value& output)
        {
            if (input == TriggerType::TriggerTypeHTTP)
            {
                output = Json::Value("HTTP");
                return;
            }
            if (input == TriggerType::TriggerTypeQueue)
            {
                output = Json::Value("Queue");
                return;
            }
        }
        inline void FromJsonEnum(const Json::Value& input, TriggerType& output)
        {
            if (!input.isString())
            {
                return;
            }
            const std::string& inputStr = input.asString();
            if (inputStr == "HTTP")
            {
                output = TriggerType::TriggerTypeHTTP;
                return;
            }
            if (inputStr == "Queue")
            {
                output = TriggerType::TriggerTypeQueue;
                return;
            }
        }

        // CloudScript Classes
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

        struct EmptyResult : public PlayFabResultCommon
        {

            EmptyResult() :
                PlayFabResultCommon()
            {}

            EmptyResult(const EmptyResult&) :
                PlayFabResultCommon()
            {}

            ~EmptyResult() = default;

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

        struct ExecuteEntityCloudScriptRequest : public PlayFabRequestCommon
        {
            std::map<std::string, std::string> CustomTags;
            Boxed<EntityKey> Entity;
            std::string FunctionName;
            Json::Value FunctionParameter;
            Boxed<bool> GeneratePlayStreamEvent;
            Boxed<CloudScriptRevisionOption> RevisionSelection;
            Boxed<Int32> SpecificRevision;

            ExecuteEntityCloudScriptRequest() :
                PlayFabRequestCommon(),
                CustomTags(),
                Entity(),
                FunctionName(),
                FunctionParameter(),
                GeneratePlayStreamEvent(),
                RevisionSelection(),
                SpecificRevision()
            {}

            ExecuteEntityCloudScriptRequest(const ExecuteEntityCloudScriptRequest& src) :
                PlayFabRequestCommon(),
                CustomTags(src.CustomTags),
                Entity(src.Entity),
                FunctionName(src.FunctionName),
                FunctionParameter(src.FunctionParameter),
                GeneratePlayStreamEvent(src.GeneratePlayStreamEvent),
                RevisionSelection(src.RevisionSelection),
                SpecificRevision(src.SpecificRevision)
            {}

            ~ExecuteEntityCloudScriptRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["CustomTags"], CustomTags);
                FromJsonUtilO(input["Entity"], Entity);
                FromJsonUtilS(input["FunctionName"], FunctionName);
                FunctionParameter = input["FunctionParameter"];
                FromJsonUtilP(input["GeneratePlayStreamEvent"], GeneratePlayStreamEvent);
                FromJsonUtilE(input["RevisionSelection"], RevisionSelection);
                FromJsonUtilP(input["SpecificRevision"], SpecificRevision);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_CustomTags; ToJsonUtilS(CustomTags, each_CustomTags); output["CustomTags"] = each_CustomTags;
                Json::Value each_Entity; ToJsonUtilO(Entity, each_Entity); output["Entity"] = each_Entity;
                Json::Value each_FunctionName; ToJsonUtilS(FunctionName, each_FunctionName); output["FunctionName"] = each_FunctionName;
                output["FunctionParameter"] = FunctionParameter;
                Json::Value each_GeneratePlayStreamEvent; ToJsonUtilP(GeneratePlayStreamEvent, each_GeneratePlayStreamEvent); output["GeneratePlayStreamEvent"] = each_GeneratePlayStreamEvent;
                Json::Value each_RevisionSelection; ToJsonUtilE(RevisionSelection, each_RevisionSelection); output["RevisionSelection"] = each_RevisionSelection;
                Json::Value each_SpecificRevision; ToJsonUtilP(SpecificRevision, each_SpecificRevision); output["SpecificRevision"] = each_SpecificRevision;
                return output;
            }
        };

        struct ExecuteFunctionRequest : public PlayFabRequestCommon
        {
            std::map<std::string, std::string> CustomTags;
            Boxed<EntityKey> Entity;
            std::string FunctionName;
            Json::Value FunctionParameter;
            Boxed<bool> GeneratePlayStreamEvent;

            ExecuteFunctionRequest() :
                PlayFabRequestCommon(),
                CustomTags(),
                Entity(),
                FunctionName(),
                FunctionParameter(),
                GeneratePlayStreamEvent()
            {}

            ExecuteFunctionRequest(const ExecuteFunctionRequest& src) :
                PlayFabRequestCommon(),
                CustomTags(src.CustomTags),
                Entity(src.Entity),
                FunctionName(src.FunctionName),
                FunctionParameter(src.FunctionParameter),
                GeneratePlayStreamEvent(src.GeneratePlayStreamEvent)
            {}

            ~ExecuteFunctionRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["CustomTags"], CustomTags);
                FromJsonUtilO(input["Entity"], Entity);
                FromJsonUtilS(input["FunctionName"], FunctionName);
                FunctionParameter = input["FunctionParameter"];
                FromJsonUtilP(input["GeneratePlayStreamEvent"], GeneratePlayStreamEvent);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_CustomTags; ToJsonUtilS(CustomTags, each_CustomTags); output["CustomTags"] = each_CustomTags;
                Json::Value each_Entity; ToJsonUtilO(Entity, each_Entity); output["Entity"] = each_Entity;
                Json::Value each_FunctionName; ToJsonUtilS(FunctionName, each_FunctionName); output["FunctionName"] = each_FunctionName;
                output["FunctionParameter"] = FunctionParameter;
                Json::Value each_GeneratePlayStreamEvent; ToJsonUtilP(GeneratePlayStreamEvent, each_GeneratePlayStreamEvent); output["GeneratePlayStreamEvent"] = each_GeneratePlayStreamEvent;
                return output;
            }
        };

        struct FunctionExecutionError : public PlayFabBaseModel
        {
            std::string Error;
            std::string Message;
            std::string StackTrace;

            FunctionExecutionError() :
                PlayFabBaseModel(),
                Error(),
                Message(),
                StackTrace()
            {}

            FunctionExecutionError(const FunctionExecutionError& src) :
                PlayFabBaseModel(),
                Error(src.Error),
                Message(src.Message),
                StackTrace(src.StackTrace)
            {}

            ~FunctionExecutionError() = default;

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

        struct ExecuteFunctionResult : public PlayFabResultCommon
        {
            Boxed<FunctionExecutionError> Error;
            Int32 ExecutionTimeMilliseconds;
            std::string FunctionName;
            Json::Value FunctionResult;
            Boxed<bool> FunctionResultTooLarge;

            ExecuteFunctionResult() :
                PlayFabResultCommon(),
                Error(),
                ExecutionTimeMilliseconds(),
                FunctionName(),
                FunctionResult(),
                FunctionResultTooLarge()
            {}

            ExecuteFunctionResult(const ExecuteFunctionResult& src) :
                PlayFabResultCommon(),
                Error(src.Error),
                ExecutionTimeMilliseconds(src.ExecutionTimeMilliseconds),
                FunctionName(src.FunctionName),
                FunctionResult(src.FunctionResult),
                FunctionResultTooLarge(src.FunctionResultTooLarge)
            {}

            ~ExecuteFunctionResult() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilO(input["Error"], Error);
                FromJsonUtilP(input["ExecutionTimeMilliseconds"], ExecutionTimeMilliseconds);
                FromJsonUtilS(input["FunctionName"], FunctionName);
                FunctionResult = input["FunctionResult"];
                FromJsonUtilP(input["FunctionResultTooLarge"], FunctionResultTooLarge);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_Error; ToJsonUtilO(Error, each_Error); output["Error"] = each_Error;
                Json::Value each_ExecutionTimeMilliseconds; ToJsonUtilP(ExecutionTimeMilliseconds, each_ExecutionTimeMilliseconds); output["ExecutionTimeMilliseconds"] = each_ExecutionTimeMilliseconds;
                Json::Value each_FunctionName; ToJsonUtilS(FunctionName, each_FunctionName); output["FunctionName"] = each_FunctionName;
                output["FunctionResult"] = FunctionResult;
                Json::Value each_FunctionResultTooLarge; ToJsonUtilP(FunctionResultTooLarge, each_FunctionResultTooLarge); output["FunctionResultTooLarge"] = each_FunctionResultTooLarge;
                return output;
            }
        };

        struct FunctionModel : public PlayFabBaseModel
        {
            std::string FunctionAddress;
            std::string FunctionName;
            std::string TriggerType;

            FunctionModel() :
                PlayFabBaseModel(),
                FunctionAddress(),
                FunctionName(),
                TriggerType()
            {}

            FunctionModel(const FunctionModel& src) :
                PlayFabBaseModel(),
                FunctionAddress(src.FunctionAddress),
                FunctionName(src.FunctionName),
                TriggerType(src.TriggerType)
            {}

            ~FunctionModel() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["FunctionAddress"], FunctionAddress);
                FromJsonUtilS(input["FunctionName"], FunctionName);
                FromJsonUtilS(input["TriggerType"], TriggerType);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_FunctionAddress; ToJsonUtilS(FunctionAddress, each_FunctionAddress); output["FunctionAddress"] = each_FunctionAddress;
                Json::Value each_FunctionName; ToJsonUtilS(FunctionName, each_FunctionName); output["FunctionName"] = each_FunctionName;
                Json::Value each_TriggerType; ToJsonUtilS(TriggerType, each_TriggerType); output["TriggerType"] = each_TriggerType;
                return output;
            }
        };

        struct GetFunctionRequest : public PlayFabRequestCommon
        {
            std::map<std::string, std::string> CustomTags;
            std::string FunctionName;
            std::string TitleId;

            GetFunctionRequest() :
                PlayFabRequestCommon(),
                CustomTags(),
                FunctionName(),
                TitleId()
            {}

            GetFunctionRequest(const GetFunctionRequest& src) :
                PlayFabRequestCommon(),
                CustomTags(src.CustomTags),
                FunctionName(src.FunctionName),
                TitleId(src.TitleId)
            {}

            ~GetFunctionRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["CustomTags"], CustomTags);
                FromJsonUtilS(input["FunctionName"], FunctionName);
                FromJsonUtilS(input["TitleId"], TitleId);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_CustomTags; ToJsonUtilS(CustomTags, each_CustomTags); output["CustomTags"] = each_CustomTags;
                Json::Value each_FunctionName; ToJsonUtilS(FunctionName, each_FunctionName); output["FunctionName"] = each_FunctionName;
                Json::Value each_TitleId; ToJsonUtilS(TitleId, each_TitleId); output["TitleId"] = each_TitleId;
                return output;
            }
        };

        struct GetFunctionResult : public PlayFabResultCommon
        {
            std::string ConnectionString;
            std::string FunctionUrl;
            std::string QueueName;
            std::string TriggerType;

            GetFunctionResult() :
                PlayFabResultCommon(),
                ConnectionString(),
                FunctionUrl(),
                QueueName(),
                TriggerType()
            {}

            GetFunctionResult(const GetFunctionResult& src) :
                PlayFabResultCommon(),
                ConnectionString(src.ConnectionString),
                FunctionUrl(src.FunctionUrl),
                QueueName(src.QueueName),
                TriggerType(src.TriggerType)
            {}

            ~GetFunctionResult() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["ConnectionString"], ConnectionString);
                FromJsonUtilS(input["FunctionUrl"], FunctionUrl);
                FromJsonUtilS(input["QueueName"], QueueName);
                FromJsonUtilS(input["TriggerType"], TriggerType);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_ConnectionString; ToJsonUtilS(ConnectionString, each_ConnectionString); output["ConnectionString"] = each_ConnectionString;
                Json::Value each_FunctionUrl; ToJsonUtilS(FunctionUrl, each_FunctionUrl); output["FunctionUrl"] = each_FunctionUrl;
                Json::Value each_QueueName; ToJsonUtilS(QueueName, each_QueueName); output["QueueName"] = each_QueueName;
                Json::Value each_TriggerType; ToJsonUtilS(TriggerType, each_TriggerType); output["TriggerType"] = each_TriggerType;
                return output;
            }
        };

        struct HttpFunctionModel : public PlayFabBaseModel
        {
            std::string FunctionName;
            std::string FunctionUrl;

            HttpFunctionModel() :
                PlayFabBaseModel(),
                FunctionName(),
                FunctionUrl()
            {}

            HttpFunctionModel(const HttpFunctionModel& src) :
                PlayFabBaseModel(),
                FunctionName(src.FunctionName),
                FunctionUrl(src.FunctionUrl)
            {}

            ~HttpFunctionModel() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["FunctionName"], FunctionName);
                FromJsonUtilS(input["FunctionUrl"], FunctionUrl);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_FunctionName; ToJsonUtilS(FunctionName, each_FunctionName); output["FunctionName"] = each_FunctionName;
                Json::Value each_FunctionUrl; ToJsonUtilS(FunctionUrl, each_FunctionUrl); output["FunctionUrl"] = each_FunctionUrl;
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

        struct ListFunctionsRequest : public PlayFabRequestCommon
        {
            std::map<std::string, std::string> CustomTags;

            ListFunctionsRequest() :
                PlayFabRequestCommon(),
                CustomTags()
            {}

            ListFunctionsRequest(const ListFunctionsRequest& src) :
                PlayFabRequestCommon(),
                CustomTags(src.CustomTags)
            {}

            ~ListFunctionsRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["CustomTags"], CustomTags);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_CustomTags; ToJsonUtilS(CustomTags, each_CustomTags); output["CustomTags"] = each_CustomTags;
                return output;
            }
        };

        struct ListFunctionsResult : public PlayFabResultCommon
        {
            std::list<FunctionModel> Functions;

            ListFunctionsResult() :
                PlayFabResultCommon(),
                Functions()
            {}

            ListFunctionsResult(const ListFunctionsResult& src) :
                PlayFabResultCommon(),
                Functions(src.Functions)
            {}

            ~ListFunctionsResult() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilO(input["Functions"], Functions);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_Functions; ToJsonUtilO(Functions, each_Functions); output["Functions"] = each_Functions;
                return output;
            }
        };

        struct ListHttpFunctionsResult : public PlayFabResultCommon
        {
            std::list<HttpFunctionModel> Functions;

            ListHttpFunctionsResult() :
                PlayFabResultCommon(),
                Functions()
            {}

            ListHttpFunctionsResult(const ListHttpFunctionsResult& src) :
                PlayFabResultCommon(),
                Functions(src.Functions)
            {}

            ~ListHttpFunctionsResult() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilO(input["Functions"], Functions);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_Functions; ToJsonUtilO(Functions, each_Functions); output["Functions"] = each_Functions;
                return output;
            }
        };

        struct QueuedFunctionModel : public PlayFabBaseModel
        {
            std::string ConnectionString;
            std::string FunctionName;
            std::string QueueName;

            QueuedFunctionModel() :
                PlayFabBaseModel(),
                ConnectionString(),
                FunctionName(),
                QueueName()
            {}

            QueuedFunctionModel(const QueuedFunctionModel& src) :
                PlayFabBaseModel(),
                ConnectionString(src.ConnectionString),
                FunctionName(src.FunctionName),
                QueueName(src.QueueName)
            {}

            ~QueuedFunctionModel() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["ConnectionString"], ConnectionString);
                FromJsonUtilS(input["FunctionName"], FunctionName);
                FromJsonUtilS(input["QueueName"], QueueName);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_ConnectionString; ToJsonUtilS(ConnectionString, each_ConnectionString); output["ConnectionString"] = each_ConnectionString;
                Json::Value each_FunctionName; ToJsonUtilS(FunctionName, each_FunctionName); output["FunctionName"] = each_FunctionName;
                Json::Value each_QueueName; ToJsonUtilS(QueueName, each_QueueName); output["QueueName"] = each_QueueName;
                return output;
            }
        };

        struct ListQueuedFunctionsResult : public PlayFabResultCommon
        {
            std::list<QueuedFunctionModel> Functions;

            ListQueuedFunctionsResult() :
                PlayFabResultCommon(),
                Functions()
            {}

            ListQueuedFunctionsResult(const ListQueuedFunctionsResult& src) :
                PlayFabResultCommon(),
                Functions(src.Functions)
            {}

            ~ListQueuedFunctionsResult() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilO(input["Functions"], Functions);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_Functions; ToJsonUtilO(Functions, each_Functions); output["Functions"] = each_Functions;
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

        struct PlayStreamEventEnvelopeModel : public PlayFabBaseModel
        {
            std::string EntityId;
            std::string EntityType;
            std::string EventData;
            std::string EventName;
            std::string EventNamespace;
            std::string EventSettings;

            PlayStreamEventEnvelopeModel() :
                PlayFabBaseModel(),
                EntityId(),
                EntityType(),
                EventData(),
                EventName(),
                EventNamespace(),
                EventSettings()
            {}

            PlayStreamEventEnvelopeModel(const PlayStreamEventEnvelopeModel& src) :
                PlayFabBaseModel(),
                EntityId(src.EntityId),
                EntityType(src.EntityType),
                EventData(src.EventData),
                EventName(src.EventName),
                EventNamespace(src.EventNamespace),
                EventSettings(src.EventSettings)
            {}

            ~PlayStreamEventEnvelopeModel() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["EntityId"], EntityId);
                FromJsonUtilS(input["EntityType"], EntityType);
                FromJsonUtilS(input["EventData"], EventData);
                FromJsonUtilS(input["EventName"], EventName);
                FromJsonUtilS(input["EventNamespace"], EventNamespace);
                FromJsonUtilS(input["EventSettings"], EventSettings);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_EntityId; ToJsonUtilS(EntityId, each_EntityId); output["EntityId"] = each_EntityId;
                Json::Value each_EntityType; ToJsonUtilS(EntityType, each_EntityType); output["EntityType"] = each_EntityType;
                Json::Value each_EventData; ToJsonUtilS(EventData, each_EventData); output["EventData"] = each_EventData;
                Json::Value each_EventName; ToJsonUtilS(EventName, each_EventName); output["EventName"] = each_EventName;
                Json::Value each_EventNamespace; ToJsonUtilS(EventNamespace, each_EventNamespace); output["EventNamespace"] = each_EventNamespace;
                Json::Value each_EventSettings; ToJsonUtilS(EventSettings, each_EventSettings); output["EventSettings"] = each_EventSettings;
                return output;
            }
        };

        struct PostFunctionResultForEntityTriggeredActionRequest : public PlayFabRequestCommon
        {
            std::map<std::string, std::string> CustomTags;
            EntityKey Entity;
            ExecuteFunctionResult FunctionResult;

            PostFunctionResultForEntityTriggeredActionRequest() :
                PlayFabRequestCommon(),
                CustomTags(),
                Entity(),
                FunctionResult()
            {}

            PostFunctionResultForEntityTriggeredActionRequest(const PostFunctionResultForEntityTriggeredActionRequest& src) :
                PlayFabRequestCommon(),
                CustomTags(src.CustomTags),
                Entity(src.Entity),
                FunctionResult(src.FunctionResult)
            {}

            ~PostFunctionResultForEntityTriggeredActionRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["CustomTags"], CustomTags);
                FromJsonUtilO(input["Entity"], Entity);
                FromJsonUtilO(input["FunctionResult"], FunctionResult);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_CustomTags; ToJsonUtilS(CustomTags, each_CustomTags); output["CustomTags"] = each_CustomTags;
                Json::Value each_Entity; ToJsonUtilO(Entity, each_Entity); output["Entity"] = each_Entity;
                Json::Value each_FunctionResult; ToJsonUtilO(FunctionResult, each_FunctionResult); output["FunctionResult"] = each_FunctionResult;
                return output;
            }
        };

        struct PostFunctionResultForFunctionExecutionRequest : public PlayFabRequestCommon
        {
            std::map<std::string, std::string> CustomTags;
            EntityKey Entity;
            ExecuteFunctionResult FunctionResult;

            PostFunctionResultForFunctionExecutionRequest() :
                PlayFabRequestCommon(),
                CustomTags(),
                Entity(),
                FunctionResult()
            {}

            PostFunctionResultForFunctionExecutionRequest(const PostFunctionResultForFunctionExecutionRequest& src) :
                PlayFabRequestCommon(),
                CustomTags(src.CustomTags),
                Entity(src.Entity),
                FunctionResult(src.FunctionResult)
            {}

            ~PostFunctionResultForFunctionExecutionRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["CustomTags"], CustomTags);
                FromJsonUtilO(input["Entity"], Entity);
                FromJsonUtilO(input["FunctionResult"], FunctionResult);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_CustomTags; ToJsonUtilS(CustomTags, each_CustomTags); output["CustomTags"] = each_CustomTags;
                Json::Value each_Entity; ToJsonUtilO(Entity, each_Entity); output["Entity"] = each_Entity;
                Json::Value each_FunctionResult; ToJsonUtilO(FunctionResult, each_FunctionResult); output["FunctionResult"] = each_FunctionResult;
                return output;
            }
        };

        struct PostFunctionResultForPlayerTriggeredActionRequest : public PlayFabRequestCommon
        {
            std::map<std::string, std::string> CustomTags;
            Boxed<EntityKey> Entity;
            ExecuteFunctionResult FunctionResult;
            PlayerProfileModel PlayerProfile;
            Boxed<PlayStreamEventEnvelopeModel> PlayStreamEventEnvelope;

            PostFunctionResultForPlayerTriggeredActionRequest() :
                PlayFabRequestCommon(),
                CustomTags(),
                Entity(),
                FunctionResult(),
                PlayerProfile(),
                PlayStreamEventEnvelope()
            {}

            PostFunctionResultForPlayerTriggeredActionRequest(const PostFunctionResultForPlayerTriggeredActionRequest& src) :
                PlayFabRequestCommon(),
                CustomTags(src.CustomTags),
                Entity(src.Entity),
                FunctionResult(src.FunctionResult),
                PlayerProfile(src.PlayerProfile),
                PlayStreamEventEnvelope(src.PlayStreamEventEnvelope)
            {}

            ~PostFunctionResultForPlayerTriggeredActionRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["CustomTags"], CustomTags);
                FromJsonUtilO(input["Entity"], Entity);
                FromJsonUtilO(input["FunctionResult"], FunctionResult);
                FromJsonUtilO(input["PlayerProfile"], PlayerProfile);
                FromJsonUtilO(input["PlayStreamEventEnvelope"], PlayStreamEventEnvelope);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_CustomTags; ToJsonUtilS(CustomTags, each_CustomTags); output["CustomTags"] = each_CustomTags;
                Json::Value each_Entity; ToJsonUtilO(Entity, each_Entity); output["Entity"] = each_Entity;
                Json::Value each_FunctionResult; ToJsonUtilO(FunctionResult, each_FunctionResult); output["FunctionResult"] = each_FunctionResult;
                Json::Value each_PlayerProfile; ToJsonUtilO(PlayerProfile, each_PlayerProfile); output["PlayerProfile"] = each_PlayerProfile;
                Json::Value each_PlayStreamEventEnvelope; ToJsonUtilO(PlayStreamEventEnvelope, each_PlayStreamEventEnvelope); output["PlayStreamEventEnvelope"] = each_PlayStreamEventEnvelope;
                return output;
            }
        };

        struct PostFunctionResultForScheduledTaskRequest : public PlayFabRequestCommon
        {
            std::map<std::string, std::string> CustomTags;
            EntityKey Entity;
            ExecuteFunctionResult FunctionResult;
            NameIdentifier ScheduledTaskId;

            PostFunctionResultForScheduledTaskRequest() :
                PlayFabRequestCommon(),
                CustomTags(),
                Entity(),
                FunctionResult(),
                ScheduledTaskId()
            {}

            PostFunctionResultForScheduledTaskRequest(const PostFunctionResultForScheduledTaskRequest& src) :
                PlayFabRequestCommon(),
                CustomTags(src.CustomTags),
                Entity(src.Entity),
                FunctionResult(src.FunctionResult),
                ScheduledTaskId(src.ScheduledTaskId)
            {}

            ~PostFunctionResultForScheduledTaskRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["CustomTags"], CustomTags);
                FromJsonUtilO(input["Entity"], Entity);
                FromJsonUtilO(input["FunctionResult"], FunctionResult);
                FromJsonUtilO(input["ScheduledTaskId"], ScheduledTaskId);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_CustomTags; ToJsonUtilS(CustomTags, each_CustomTags); output["CustomTags"] = each_CustomTags;
                Json::Value each_Entity; ToJsonUtilO(Entity, each_Entity); output["Entity"] = each_Entity;
                Json::Value each_FunctionResult; ToJsonUtilO(FunctionResult, each_FunctionResult); output["FunctionResult"] = each_FunctionResult;
                Json::Value each_ScheduledTaskId; ToJsonUtilO(ScheduledTaskId, each_ScheduledTaskId); output["ScheduledTaskId"] = each_ScheduledTaskId;
                return output;
            }
        };

        struct RegisterHttpFunctionRequest : public PlayFabRequestCommon
        {
            std::string AzureResourceId;
            std::map<std::string, std::string> CustomTags;
            std::string FunctionName;
            std::string FunctionUrl;
            std::string TitleId;

            RegisterHttpFunctionRequest() :
                PlayFabRequestCommon(),
                AzureResourceId(),
                CustomTags(),
                FunctionName(),
                FunctionUrl(),
                TitleId()
            {}

            RegisterHttpFunctionRequest(const RegisterHttpFunctionRequest& src) :
                PlayFabRequestCommon(),
                AzureResourceId(src.AzureResourceId),
                CustomTags(src.CustomTags),
                FunctionName(src.FunctionName),
                FunctionUrl(src.FunctionUrl),
                TitleId(src.TitleId)
            {}

            ~RegisterHttpFunctionRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["AzureResourceId"], AzureResourceId);
                FromJsonUtilS(input["CustomTags"], CustomTags);
                FromJsonUtilS(input["FunctionName"], FunctionName);
                FromJsonUtilS(input["FunctionUrl"], FunctionUrl);
                FromJsonUtilS(input["TitleId"], TitleId);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_AzureResourceId; ToJsonUtilS(AzureResourceId, each_AzureResourceId); output["AzureResourceId"] = each_AzureResourceId;
                Json::Value each_CustomTags; ToJsonUtilS(CustomTags, each_CustomTags); output["CustomTags"] = each_CustomTags;
                Json::Value each_FunctionName; ToJsonUtilS(FunctionName, each_FunctionName); output["FunctionName"] = each_FunctionName;
                Json::Value each_FunctionUrl; ToJsonUtilS(FunctionUrl, each_FunctionUrl); output["FunctionUrl"] = each_FunctionUrl;
                Json::Value each_TitleId; ToJsonUtilS(TitleId, each_TitleId); output["TitleId"] = each_TitleId;
                return output;
            }
        };

        struct RegisterQueuedFunctionRequest : public PlayFabRequestCommon
        {
            std::string AzureResourceId;
            std::string ConnectionString;
            std::map<std::string, std::string> CustomTags;
            std::string FunctionName;
            std::string QueueName;
            std::string TitleId;

            RegisterQueuedFunctionRequest() :
                PlayFabRequestCommon(),
                AzureResourceId(),
                ConnectionString(),
                CustomTags(),
                FunctionName(),
                QueueName(),
                TitleId()
            {}

            RegisterQueuedFunctionRequest(const RegisterQueuedFunctionRequest& src) :
                PlayFabRequestCommon(),
                AzureResourceId(src.AzureResourceId),
                ConnectionString(src.ConnectionString),
                CustomTags(src.CustomTags),
                FunctionName(src.FunctionName),
                QueueName(src.QueueName),
                TitleId(src.TitleId)
            {}

            ~RegisterQueuedFunctionRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["AzureResourceId"], AzureResourceId);
                FromJsonUtilS(input["ConnectionString"], ConnectionString);
                FromJsonUtilS(input["CustomTags"], CustomTags);
                FromJsonUtilS(input["FunctionName"], FunctionName);
                FromJsonUtilS(input["QueueName"], QueueName);
                FromJsonUtilS(input["TitleId"], TitleId);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_AzureResourceId; ToJsonUtilS(AzureResourceId, each_AzureResourceId); output["AzureResourceId"] = each_AzureResourceId;
                Json::Value each_ConnectionString; ToJsonUtilS(ConnectionString, each_ConnectionString); output["ConnectionString"] = each_ConnectionString;
                Json::Value each_CustomTags; ToJsonUtilS(CustomTags, each_CustomTags); output["CustomTags"] = each_CustomTags;
                Json::Value each_FunctionName; ToJsonUtilS(FunctionName, each_FunctionName); output["FunctionName"] = each_FunctionName;
                Json::Value each_QueueName; ToJsonUtilS(QueueName, each_QueueName); output["QueueName"] = each_QueueName;
                Json::Value each_TitleId; ToJsonUtilS(TitleId, each_TitleId); output["TitleId"] = each_TitleId;
                return output;
            }
        };

        struct UnregisterFunctionRequest : public PlayFabRequestCommon
        {
            std::map<std::string, std::string> CustomTags;
            std::string FunctionName;
            std::string TitleId;

            UnregisterFunctionRequest() :
                PlayFabRequestCommon(),
                CustomTags(),
                FunctionName(),
                TitleId()
            {}

            UnregisterFunctionRequest(const UnregisterFunctionRequest& src) :
                PlayFabRequestCommon(),
                CustomTags(src.CustomTags),
                FunctionName(src.FunctionName),
                TitleId(src.TitleId)
            {}

            ~UnregisterFunctionRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["CustomTags"], CustomTags);
                FromJsonUtilS(input["FunctionName"], FunctionName);
                FromJsonUtilS(input["TitleId"], TitleId);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_CustomTags; ToJsonUtilS(CustomTags, each_CustomTags); output["CustomTags"] = each_CustomTags;
                Json::Value each_FunctionName; ToJsonUtilS(FunctionName, each_FunctionName); output["FunctionName"] = each_FunctionName;
                Json::Value each_TitleId; ToJsonUtilS(TitleId, each_TitleId); output["TitleId"] = each_TitleId;
                return output;
            }
        };

    }
}

#endif
