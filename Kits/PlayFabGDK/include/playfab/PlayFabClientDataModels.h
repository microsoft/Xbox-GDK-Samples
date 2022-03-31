#pragma once

#if !defined(DISABLE_PLAYFABCLIENT_API)

#include <playfab/PlayFabBaseModel.h>
#include <playfab/PlayFabJsonHeaders.h>

namespace PlayFab
{
    namespace ClientModels
    {
        // Client Enums
        enum class AdActivity
        {
            AdActivityOpened,
            AdActivityClosed,
            AdActivityStart,
            AdActivityEnd
        };

        inline void ToJsonEnum(const AdActivity input, Json::Value& output)
        {
            if (input == AdActivity::AdActivityOpened)
            {
                output = Json::Value("Opened");
                return;
            }
            if (input == AdActivity::AdActivityClosed)
            {
                output = Json::Value("Closed");
                return;
            }
            if (input == AdActivity::AdActivityStart)
            {
                output = Json::Value("Start");
                return;
            }
            if (input == AdActivity::AdActivityEnd)
            {
                output = Json::Value("End");
                return;
            }
        }
        inline void FromJsonEnum(const Json::Value& input, AdActivity& output)
        {
            if (!input.isString())
            {
                return;
            }
            const std::string& inputStr = input.asString();
            if (inputStr == "Opened")
            {
                output = AdActivity::AdActivityOpened;
                return;
            }
            if (inputStr == "Closed")
            {
                output = AdActivity::AdActivityClosed;
                return;
            }
            if (inputStr == "Start")
            {
                output = AdActivity::AdActivityStart;
                return;
            }
            if (inputStr == "End")
            {
                output = AdActivity::AdActivityEnd;
                return;
            }
        }

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

        enum class GameInstanceState
        {
            GameInstanceStateOpen,
            GameInstanceStateClosed
        };

        inline void ToJsonEnum(const GameInstanceState input, Json::Value& output)
        {
            if (input == GameInstanceState::GameInstanceStateOpen)
            {
                output = Json::Value("Open");
                return;
            }
            if (input == GameInstanceState::GameInstanceStateClosed)
            {
                output = Json::Value("Closed");
                return;
            }
        }
        inline void FromJsonEnum(const Json::Value& input, GameInstanceState& output)
        {
            if (!input.isString())
            {
                return;
            }
            const std::string& inputStr = input.asString();
            if (inputStr == "Open")
            {
                output = GameInstanceState::GameInstanceStateOpen;
                return;
            }
            if (inputStr == "Closed")
            {
                output = GameInstanceState::GameInstanceStateClosed;
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

        enum class MatchmakeStatus
        {
            MatchmakeStatusComplete,
            MatchmakeStatusWaiting,
            MatchmakeStatusGameNotFound,
            MatchmakeStatusNoAvailableSlots,
            MatchmakeStatusSessionClosed
        };

        inline void ToJsonEnum(const MatchmakeStatus input, Json::Value& output)
        {
            if (input == MatchmakeStatus::MatchmakeStatusComplete)
            {
                output = Json::Value("Complete");
                return;
            }
            if (input == MatchmakeStatus::MatchmakeStatusWaiting)
            {
                output = Json::Value("Waiting");
                return;
            }
            if (input == MatchmakeStatus::MatchmakeStatusGameNotFound)
            {
                output = Json::Value("GameNotFound");
                return;
            }
            if (input == MatchmakeStatus::MatchmakeStatusNoAvailableSlots)
            {
                output = Json::Value("NoAvailableSlots");
                return;
            }
            if (input == MatchmakeStatus::MatchmakeStatusSessionClosed)
            {
                output = Json::Value("SessionClosed");
                return;
            }
        }
        inline void FromJsonEnum(const Json::Value& input, MatchmakeStatus& output)
        {
            if (!input.isString())
            {
                return;
            }
            const std::string& inputStr = input.asString();
            if (inputStr == "Complete")
            {
                output = MatchmakeStatus::MatchmakeStatusComplete;
                return;
            }
            if (inputStr == "Waiting")
            {
                output = MatchmakeStatus::MatchmakeStatusWaiting;
                return;
            }
            if (inputStr == "GameNotFound")
            {
                output = MatchmakeStatus::MatchmakeStatusGameNotFound;
                return;
            }
            if (inputStr == "NoAvailableSlots")
            {
                output = MatchmakeStatus::MatchmakeStatusNoAvailableSlots;
                return;
            }
            if (inputStr == "SessionClosed")
            {
                output = MatchmakeStatus::MatchmakeStatusSessionClosed;
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

        enum class TradeStatus
        {
            TradeStatusInvalid,
            TradeStatusOpening,
            TradeStatusOpen,
            TradeStatusAccepting,
            TradeStatusAccepted,
            TradeStatusFilled,
            TradeStatusCancelled
        };

        inline void ToJsonEnum(const TradeStatus input, Json::Value& output)
        {
            if (input == TradeStatus::TradeStatusInvalid)
            {
                output = Json::Value("Invalid");
                return;
            }
            if (input == TradeStatus::TradeStatusOpening)
            {
                output = Json::Value("Opening");
                return;
            }
            if (input == TradeStatus::TradeStatusOpen)
            {
                output = Json::Value("Open");
                return;
            }
            if (input == TradeStatus::TradeStatusAccepting)
            {
                output = Json::Value("Accepting");
                return;
            }
            if (input == TradeStatus::TradeStatusAccepted)
            {
                output = Json::Value("Accepted");
                return;
            }
            if (input == TradeStatus::TradeStatusFilled)
            {
                output = Json::Value("Filled");
                return;
            }
            if (input == TradeStatus::TradeStatusCancelled)
            {
                output = Json::Value("Cancelled");
                return;
            }
        }
        inline void FromJsonEnum(const Json::Value& input, TradeStatus& output)
        {
            if (!input.isString())
            {
                return;
            }
            const std::string& inputStr = input.asString();
            if (inputStr == "Invalid")
            {
                output = TradeStatus::TradeStatusInvalid;
                return;
            }
            if (inputStr == "Opening")
            {
                output = TradeStatus::TradeStatusOpening;
                return;
            }
            if (inputStr == "Open")
            {
                output = TradeStatus::TradeStatusOpen;
                return;
            }
            if (inputStr == "Accepting")
            {
                output = TradeStatus::TradeStatusAccepting;
                return;
            }
            if (inputStr == "Accepted")
            {
                output = TradeStatus::TradeStatusAccepted;
                return;
            }
            if (inputStr == "Filled")
            {
                output = TradeStatus::TradeStatusFilled;
                return;
            }
            if (inputStr == "Cancelled")
            {
                output = TradeStatus::TradeStatusCancelled;
                return;
            }
        }

        enum class TransactionStatus
        {
            TransactionStatusCreateCart,
            TransactionStatusInit,
            TransactionStatusApproved,
            TransactionStatusSucceeded,
            TransactionStatusFailedByProvider,
            TransactionStatusDisputePending,
            TransactionStatusRefundPending,
            TransactionStatusRefunded,
            TransactionStatusRefundFailed,
            TransactionStatusChargedBack,
            TransactionStatusFailedByUber,
            TransactionStatusFailedByPlayFab,
            TransactionStatusRevoked,
            TransactionStatusTradePending,
            TransactionStatusTraded,
            TransactionStatusUpgraded,
            TransactionStatusStackPending,
            TransactionStatusStacked,
            TransactionStatusOther,
            TransactionStatusFailed
        };

        inline void ToJsonEnum(const TransactionStatus input, Json::Value& output)
        {
            if (input == TransactionStatus::TransactionStatusCreateCart)
            {
                output = Json::Value("CreateCart");
                return;
            }
            if (input == TransactionStatus::TransactionStatusInit)
            {
                output = Json::Value("Init");
                return;
            }
            if (input == TransactionStatus::TransactionStatusApproved)
            {
                output = Json::Value("Approved");
                return;
            }
            if (input == TransactionStatus::TransactionStatusSucceeded)
            {
                output = Json::Value("Succeeded");
                return;
            }
            if (input == TransactionStatus::TransactionStatusFailedByProvider)
            {
                output = Json::Value("FailedByProvider");
                return;
            }
            if (input == TransactionStatus::TransactionStatusDisputePending)
            {
                output = Json::Value("DisputePending");
                return;
            }
            if (input == TransactionStatus::TransactionStatusRefundPending)
            {
                output = Json::Value("RefundPending");
                return;
            }
            if (input == TransactionStatus::TransactionStatusRefunded)
            {
                output = Json::Value("Refunded");
                return;
            }
            if (input == TransactionStatus::TransactionStatusRefundFailed)
            {
                output = Json::Value("RefundFailed");
                return;
            }
            if (input == TransactionStatus::TransactionStatusChargedBack)
            {
                output = Json::Value("ChargedBack");
                return;
            }
            if (input == TransactionStatus::TransactionStatusFailedByUber)
            {
                output = Json::Value("FailedByUber");
                return;
            }
            if (input == TransactionStatus::TransactionStatusFailedByPlayFab)
            {
                output = Json::Value("FailedByPlayFab");
                return;
            }
            if (input == TransactionStatus::TransactionStatusRevoked)
            {
                output = Json::Value("Revoked");
                return;
            }
            if (input == TransactionStatus::TransactionStatusTradePending)
            {
                output = Json::Value("TradePending");
                return;
            }
            if (input == TransactionStatus::TransactionStatusTraded)
            {
                output = Json::Value("Traded");
                return;
            }
            if (input == TransactionStatus::TransactionStatusUpgraded)
            {
                output = Json::Value("Upgraded");
                return;
            }
            if (input == TransactionStatus::TransactionStatusStackPending)
            {
                output = Json::Value("StackPending");
                return;
            }
            if (input == TransactionStatus::TransactionStatusStacked)
            {
                output = Json::Value("Stacked");
                return;
            }
            if (input == TransactionStatus::TransactionStatusOther)
            {
                output = Json::Value("Other");
                return;
            }
            if (input == TransactionStatus::TransactionStatusFailed)
            {
                output = Json::Value("Failed");
                return;
            }
        }
        inline void FromJsonEnum(const Json::Value& input, TransactionStatus& output)
        {
            if (!input.isString())
            {
                return;
            }
            const std::string& inputStr = input.asString();
            if (inputStr == "CreateCart")
            {
                output = TransactionStatus::TransactionStatusCreateCart;
                return;
            }
            if (inputStr == "Init")
            {
                output = TransactionStatus::TransactionStatusInit;
                return;
            }
            if (inputStr == "Approved")
            {
                output = TransactionStatus::TransactionStatusApproved;
                return;
            }
            if (inputStr == "Succeeded")
            {
                output = TransactionStatus::TransactionStatusSucceeded;
                return;
            }
            if (inputStr == "FailedByProvider")
            {
                output = TransactionStatus::TransactionStatusFailedByProvider;
                return;
            }
            if (inputStr == "DisputePending")
            {
                output = TransactionStatus::TransactionStatusDisputePending;
                return;
            }
            if (inputStr == "RefundPending")
            {
                output = TransactionStatus::TransactionStatusRefundPending;
                return;
            }
            if (inputStr == "Refunded")
            {
                output = TransactionStatus::TransactionStatusRefunded;
                return;
            }
            if (inputStr == "RefundFailed")
            {
                output = TransactionStatus::TransactionStatusRefundFailed;
                return;
            }
            if (inputStr == "ChargedBack")
            {
                output = TransactionStatus::TransactionStatusChargedBack;
                return;
            }
            if (inputStr == "FailedByUber")
            {
                output = TransactionStatus::TransactionStatusFailedByUber;
                return;
            }
            if (inputStr == "FailedByPlayFab")
            {
                output = TransactionStatus::TransactionStatusFailedByPlayFab;
                return;
            }
            if (inputStr == "Revoked")
            {
                output = TransactionStatus::TransactionStatusRevoked;
                return;
            }
            if (inputStr == "TradePending")
            {
                output = TransactionStatus::TransactionStatusTradePending;
                return;
            }
            if (inputStr == "Traded")
            {
                output = TransactionStatus::TransactionStatusTraded;
                return;
            }
            if (inputStr == "Upgraded")
            {
                output = TransactionStatus::TransactionStatusUpgraded;
                return;
            }
            if (inputStr == "StackPending")
            {
                output = TransactionStatus::TransactionStatusStackPending;
                return;
            }
            if (inputStr == "Stacked")
            {
                output = TransactionStatus::TransactionStatusStacked;
                return;
            }
            if (inputStr == "Other")
            {
                output = TransactionStatus::TransactionStatusOther;
                return;
            }
            if (inputStr == "Failed")
            {
                output = TransactionStatus::TransactionStatusFailed;
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

        // Client Classes
        struct AcceptTradeRequest : public PlayFabRequestCommon
        {
            std::list<std::string> AcceptedInventoryInstanceIds;
            std::string OfferingPlayerId;
            std::string TradeId;

            AcceptTradeRequest() :
                PlayFabRequestCommon(),
                AcceptedInventoryInstanceIds(),
                OfferingPlayerId(),
                TradeId()
            {}

            AcceptTradeRequest(const AcceptTradeRequest& src) :
                PlayFabRequestCommon(),
                AcceptedInventoryInstanceIds(src.AcceptedInventoryInstanceIds),
                OfferingPlayerId(src.OfferingPlayerId),
                TradeId(src.TradeId)
            {}

            ~AcceptTradeRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["AcceptedInventoryInstanceIds"], AcceptedInventoryInstanceIds);
                FromJsonUtilS(input["OfferingPlayerId"], OfferingPlayerId);
                FromJsonUtilS(input["TradeId"], TradeId);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_AcceptedInventoryInstanceIds; ToJsonUtilS(AcceptedInventoryInstanceIds, each_AcceptedInventoryInstanceIds); output["AcceptedInventoryInstanceIds"] = each_AcceptedInventoryInstanceIds;
                Json::Value each_OfferingPlayerId; ToJsonUtilS(OfferingPlayerId, each_OfferingPlayerId); output["OfferingPlayerId"] = each_OfferingPlayerId;
                Json::Value each_TradeId; ToJsonUtilS(TradeId, each_TradeId); output["TradeId"] = each_TradeId;
                return output;
            }
        };

        struct TradeInfo : public PlayFabBaseModel
        {
            std::list<std::string> AcceptedInventoryInstanceIds;
            std::string AcceptedPlayerId;
            std::list<std::string> AllowedPlayerIds;
            Boxed<time_t> CancelledAt;
            Boxed<time_t> FilledAt;
            Boxed<time_t> InvalidatedAt;
            std::list<std::string> OfferedCatalogItemIds;
            std::list<std::string> OfferedInventoryInstanceIds;
            std::string OfferingPlayerId;
            Boxed<time_t> OpenedAt;
            std::list<std::string> RequestedCatalogItemIds;
            Boxed<TradeStatus> Status;
            std::string TradeId;

            TradeInfo() :
                PlayFabBaseModel(),
                AcceptedInventoryInstanceIds(),
                AcceptedPlayerId(),
                AllowedPlayerIds(),
                CancelledAt(),
                FilledAt(),
                InvalidatedAt(),
                OfferedCatalogItemIds(),
                OfferedInventoryInstanceIds(),
                OfferingPlayerId(),
                OpenedAt(),
                RequestedCatalogItemIds(),
                Status(),
                TradeId()
            {}

            TradeInfo(const TradeInfo& src) :
                PlayFabBaseModel(),
                AcceptedInventoryInstanceIds(src.AcceptedInventoryInstanceIds),
                AcceptedPlayerId(src.AcceptedPlayerId),
                AllowedPlayerIds(src.AllowedPlayerIds),
                CancelledAt(src.CancelledAt),
                FilledAt(src.FilledAt),
                InvalidatedAt(src.InvalidatedAt),
                OfferedCatalogItemIds(src.OfferedCatalogItemIds),
                OfferedInventoryInstanceIds(src.OfferedInventoryInstanceIds),
                OfferingPlayerId(src.OfferingPlayerId),
                OpenedAt(src.OpenedAt),
                RequestedCatalogItemIds(src.RequestedCatalogItemIds),
                Status(src.Status),
                TradeId(src.TradeId)
            {}

            ~TradeInfo() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["AcceptedInventoryInstanceIds"], AcceptedInventoryInstanceIds);
                FromJsonUtilS(input["AcceptedPlayerId"], AcceptedPlayerId);
                FromJsonUtilS(input["AllowedPlayerIds"], AllowedPlayerIds);
                FromJsonUtilT(input["CancelledAt"], CancelledAt);
                FromJsonUtilT(input["FilledAt"], FilledAt);
                FromJsonUtilT(input["InvalidatedAt"], InvalidatedAt);
                FromJsonUtilS(input["OfferedCatalogItemIds"], OfferedCatalogItemIds);
                FromJsonUtilS(input["OfferedInventoryInstanceIds"], OfferedInventoryInstanceIds);
                FromJsonUtilS(input["OfferingPlayerId"], OfferingPlayerId);
                FromJsonUtilT(input["OpenedAt"], OpenedAt);
                FromJsonUtilS(input["RequestedCatalogItemIds"], RequestedCatalogItemIds);
                FromJsonUtilE(input["Status"], Status);
                FromJsonUtilS(input["TradeId"], TradeId);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_AcceptedInventoryInstanceIds; ToJsonUtilS(AcceptedInventoryInstanceIds, each_AcceptedInventoryInstanceIds); output["AcceptedInventoryInstanceIds"] = each_AcceptedInventoryInstanceIds;
                Json::Value each_AcceptedPlayerId; ToJsonUtilS(AcceptedPlayerId, each_AcceptedPlayerId); output["AcceptedPlayerId"] = each_AcceptedPlayerId;
                Json::Value each_AllowedPlayerIds; ToJsonUtilS(AllowedPlayerIds, each_AllowedPlayerIds); output["AllowedPlayerIds"] = each_AllowedPlayerIds;
                Json::Value each_CancelledAt; ToJsonUtilT(CancelledAt, each_CancelledAt); output["CancelledAt"] = each_CancelledAt;
                Json::Value each_FilledAt; ToJsonUtilT(FilledAt, each_FilledAt); output["FilledAt"] = each_FilledAt;
                Json::Value each_InvalidatedAt; ToJsonUtilT(InvalidatedAt, each_InvalidatedAt); output["InvalidatedAt"] = each_InvalidatedAt;
                Json::Value each_OfferedCatalogItemIds; ToJsonUtilS(OfferedCatalogItemIds, each_OfferedCatalogItemIds); output["OfferedCatalogItemIds"] = each_OfferedCatalogItemIds;
                Json::Value each_OfferedInventoryInstanceIds; ToJsonUtilS(OfferedInventoryInstanceIds, each_OfferedInventoryInstanceIds); output["OfferedInventoryInstanceIds"] = each_OfferedInventoryInstanceIds;
                Json::Value each_OfferingPlayerId; ToJsonUtilS(OfferingPlayerId, each_OfferingPlayerId); output["OfferingPlayerId"] = each_OfferingPlayerId;
                Json::Value each_OpenedAt; ToJsonUtilT(OpenedAt, each_OpenedAt); output["OpenedAt"] = each_OpenedAt;
                Json::Value each_RequestedCatalogItemIds; ToJsonUtilS(RequestedCatalogItemIds, each_RequestedCatalogItemIds); output["RequestedCatalogItemIds"] = each_RequestedCatalogItemIds;
                Json::Value each_Status; ToJsonUtilE(Status, each_Status); output["Status"] = each_Status;
                Json::Value each_TradeId; ToJsonUtilS(TradeId, each_TradeId); output["TradeId"] = each_TradeId;
                return output;
            }
        };

        struct AcceptTradeResponse : public PlayFabResultCommon
        {
            Boxed<TradeInfo> Trade;

            AcceptTradeResponse() :
                PlayFabResultCommon(),
                Trade()
            {}

            AcceptTradeResponse(const AcceptTradeResponse& src) :
                PlayFabResultCommon(),
                Trade(src.Trade)
            {}

            ~AcceptTradeResponse() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilO(input["Trade"], Trade);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_Trade; ToJsonUtilO(Trade, each_Trade); output["Trade"] = each_Trade;
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

        struct AddFriendRequest : public PlayFabRequestCommon
        {
            std::string FriendEmail;
            std::string FriendPlayFabId;
            std::string FriendTitleDisplayName;
            std::string FriendUsername;

            AddFriendRequest() :
                PlayFabRequestCommon(),
                FriendEmail(),
                FriendPlayFabId(),
                FriendTitleDisplayName(),
                FriendUsername()
            {}

            AddFriendRequest(const AddFriendRequest& src) :
                PlayFabRequestCommon(),
                FriendEmail(src.FriendEmail),
                FriendPlayFabId(src.FriendPlayFabId),
                FriendTitleDisplayName(src.FriendTitleDisplayName),
                FriendUsername(src.FriendUsername)
            {}

            ~AddFriendRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["FriendEmail"], FriendEmail);
                FromJsonUtilS(input["FriendPlayFabId"], FriendPlayFabId);
                FromJsonUtilS(input["FriendTitleDisplayName"], FriendTitleDisplayName);
                FromJsonUtilS(input["FriendUsername"], FriendUsername);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_FriendEmail; ToJsonUtilS(FriendEmail, each_FriendEmail); output["FriendEmail"] = each_FriendEmail;
                Json::Value each_FriendPlayFabId; ToJsonUtilS(FriendPlayFabId, each_FriendPlayFabId); output["FriendPlayFabId"] = each_FriendPlayFabId;
                Json::Value each_FriendTitleDisplayName; ToJsonUtilS(FriendTitleDisplayName, each_FriendTitleDisplayName); output["FriendTitleDisplayName"] = each_FriendTitleDisplayName;
                Json::Value each_FriendUsername; ToJsonUtilS(FriendUsername, each_FriendUsername); output["FriendUsername"] = each_FriendUsername;
                return output;
            }
        };

        struct AddFriendResult : public PlayFabResultCommon
        {
            bool Created;

            AddFriendResult() :
                PlayFabResultCommon(),
                Created()
            {}

            AddFriendResult(const AddFriendResult& src) :
                PlayFabResultCommon(),
                Created(src.Created)
            {}

            ~AddFriendResult() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilP(input["Created"], Created);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_Created; ToJsonUtilP(Created, each_Created); output["Created"] = each_Created;
                return output;
            }
        };

        struct GenericServiceId : public PlayFabBaseModel
        {
            std::string ServiceName;
            std::string UserId;

            GenericServiceId() :
                PlayFabBaseModel(),
                ServiceName(),
                UserId()
            {}

            GenericServiceId(const GenericServiceId& src) :
                PlayFabBaseModel(),
                ServiceName(src.ServiceName),
                UserId(src.UserId)
            {}

            ~GenericServiceId() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["ServiceName"], ServiceName);
                FromJsonUtilS(input["UserId"], UserId);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_ServiceName; ToJsonUtilS(ServiceName, each_ServiceName); output["ServiceName"] = each_ServiceName;
                Json::Value each_UserId; ToJsonUtilS(UserId, each_UserId); output["UserId"] = each_UserId;
                return output;
            }
        };

        struct AddGenericIDRequest : public PlayFabRequestCommon
        {
            GenericServiceId GenericId;

            AddGenericIDRequest() :
                PlayFabRequestCommon(),
                GenericId()
            {}

            AddGenericIDRequest(const AddGenericIDRequest& src) :
                PlayFabRequestCommon(),
                GenericId(src.GenericId)
            {}

            ~AddGenericIDRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilO(input["GenericId"], GenericId);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_GenericId; ToJsonUtilO(GenericId, each_GenericId); output["GenericId"] = each_GenericId;
                return output;
            }
        };

        struct AddGenericIDResult : public PlayFabResultCommon
        {

            AddGenericIDResult() :
                PlayFabResultCommon()
            {}

            AddGenericIDResult(const AddGenericIDResult&) :
                PlayFabResultCommon()
            {}

            ~AddGenericIDResult() = default;

            void FromJson(const Json::Value&) override
            {
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                return output;
            }
        };

        struct AddOrUpdateContactEmailRequest : public PlayFabRequestCommon
        {
            std::map<std::string, std::string> CustomTags;
            std::string EmailAddress;

            AddOrUpdateContactEmailRequest() :
                PlayFabRequestCommon(),
                CustomTags(),
                EmailAddress()
            {}

            AddOrUpdateContactEmailRequest(const AddOrUpdateContactEmailRequest& src) :
                PlayFabRequestCommon(),
                CustomTags(src.CustomTags),
                EmailAddress(src.EmailAddress)
            {}

            ~AddOrUpdateContactEmailRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["CustomTags"], CustomTags);
                FromJsonUtilS(input["EmailAddress"], EmailAddress);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_CustomTags; ToJsonUtilS(CustomTags, each_CustomTags); output["CustomTags"] = each_CustomTags;
                Json::Value each_EmailAddress; ToJsonUtilS(EmailAddress, each_EmailAddress); output["EmailAddress"] = each_EmailAddress;
                return output;
            }
        };

        struct AddOrUpdateContactEmailResult : public PlayFabResultCommon
        {

            AddOrUpdateContactEmailResult() :
                PlayFabResultCommon()
            {}

            AddOrUpdateContactEmailResult(const AddOrUpdateContactEmailResult&) :
                PlayFabResultCommon()
            {}

            ~AddOrUpdateContactEmailResult() = default;

            void FromJson(const Json::Value&) override
            {
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                return output;
            }
        };

        struct AddSharedGroupMembersRequest : public PlayFabRequestCommon
        {
            std::list<std::string> PlayFabIds;
            std::string SharedGroupId;

            AddSharedGroupMembersRequest() :
                PlayFabRequestCommon(),
                PlayFabIds(),
                SharedGroupId()
            {}

            AddSharedGroupMembersRequest(const AddSharedGroupMembersRequest& src) :
                PlayFabRequestCommon(),
                PlayFabIds(src.PlayFabIds),
                SharedGroupId(src.SharedGroupId)
            {}

            ~AddSharedGroupMembersRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["PlayFabIds"], PlayFabIds);
                FromJsonUtilS(input["SharedGroupId"], SharedGroupId);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_PlayFabIds; ToJsonUtilS(PlayFabIds, each_PlayFabIds); output["PlayFabIds"] = each_PlayFabIds;
                Json::Value each_SharedGroupId; ToJsonUtilS(SharedGroupId, each_SharedGroupId); output["SharedGroupId"] = each_SharedGroupId;
                return output;
            }
        };

        struct AddSharedGroupMembersResult : public PlayFabResultCommon
        {

            AddSharedGroupMembersResult() :
                PlayFabResultCommon()
            {}

            AddSharedGroupMembersResult(const AddSharedGroupMembersResult&) :
                PlayFabResultCommon()
            {}

            ~AddSharedGroupMembersResult() = default;

            void FromJson(const Json::Value&) override
            {
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                return output;
            }
        };

        struct AddUsernamePasswordRequest : public PlayFabRequestCommon
        {
            std::map<std::string, std::string> CustomTags;
            std::string Email;
            std::string Password;
            std::string Username;

            AddUsernamePasswordRequest() :
                PlayFabRequestCommon(),
                CustomTags(),
                Email(),
                Password(),
                Username()
            {}

            AddUsernamePasswordRequest(const AddUsernamePasswordRequest& src) :
                PlayFabRequestCommon(),
                CustomTags(src.CustomTags),
                Email(src.Email),
                Password(src.Password),
                Username(src.Username)
            {}

            ~AddUsernamePasswordRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["CustomTags"], CustomTags);
                FromJsonUtilS(input["Email"], Email);
                FromJsonUtilS(input["Password"], Password);
                FromJsonUtilS(input["Username"], Username);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_CustomTags; ToJsonUtilS(CustomTags, each_CustomTags); output["CustomTags"] = each_CustomTags;
                Json::Value each_Email; ToJsonUtilS(Email, each_Email); output["Email"] = each_Email;
                Json::Value each_Password; ToJsonUtilS(Password, each_Password); output["Password"] = each_Password;
                Json::Value each_Username; ToJsonUtilS(Username, each_Username); output["Username"] = each_Username;
                return output;
            }
        };

        struct AddUsernamePasswordResult : public PlayFabResultCommon
        {
            std::string Username;

            AddUsernamePasswordResult() :
                PlayFabResultCommon(),
                Username()
            {}

            AddUsernamePasswordResult(const AddUsernamePasswordResult& src) :
                PlayFabResultCommon(),
                Username(src.Username)
            {}

            ~AddUsernamePasswordResult() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["Username"], Username);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_Username; ToJsonUtilS(Username, each_Username); output["Username"] = each_Username;
                return output;
            }
        };

        struct AddUserVirtualCurrencyRequest : public PlayFabRequestCommon
        {
            Int32 Amount;
            std::map<std::string, std::string> CustomTags;
            std::string VirtualCurrency;

            AddUserVirtualCurrencyRequest() :
                PlayFabRequestCommon(),
                Amount(),
                CustomTags(),
                VirtualCurrency()
            {}

            AddUserVirtualCurrencyRequest(const AddUserVirtualCurrencyRequest& src) :
                PlayFabRequestCommon(),
                Amount(src.Amount),
                CustomTags(src.CustomTags),
                VirtualCurrency(src.VirtualCurrency)
            {}

            ~AddUserVirtualCurrencyRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilP(input["Amount"], Amount);
                FromJsonUtilS(input["CustomTags"], CustomTags);
                FromJsonUtilS(input["VirtualCurrency"], VirtualCurrency);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_Amount; ToJsonUtilP(Amount, each_Amount); output["Amount"] = each_Amount;
                Json::Value each_CustomTags; ToJsonUtilS(CustomTags, each_CustomTags); output["CustomTags"] = each_CustomTags;
                Json::Value each_VirtualCurrency; ToJsonUtilS(VirtualCurrency, each_VirtualCurrency); output["VirtualCurrency"] = each_VirtualCurrency;
                return output;
            }
        };

        struct AdPlacementDetails : public PlayFabBaseModel
        {
            std::string PlacementId;
            std::string PlacementName;
            Boxed<Int32> PlacementViewsRemaining;
            Boxed<double> PlacementViewsResetMinutes;
            std::string RewardAssetUrl;
            std::string RewardDescription;
            std::string RewardId;
            std::string RewardName;

            AdPlacementDetails() :
                PlayFabBaseModel(),
                PlacementId(),
                PlacementName(),
                PlacementViewsRemaining(),
                PlacementViewsResetMinutes(),
                RewardAssetUrl(),
                RewardDescription(),
                RewardId(),
                RewardName()
            {}

            AdPlacementDetails(const AdPlacementDetails& src) :
                PlayFabBaseModel(),
                PlacementId(src.PlacementId),
                PlacementName(src.PlacementName),
                PlacementViewsRemaining(src.PlacementViewsRemaining),
                PlacementViewsResetMinutes(src.PlacementViewsResetMinutes),
                RewardAssetUrl(src.RewardAssetUrl),
                RewardDescription(src.RewardDescription),
                RewardId(src.RewardId),
                RewardName(src.RewardName)
            {}

            ~AdPlacementDetails() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["PlacementId"], PlacementId);
                FromJsonUtilS(input["PlacementName"], PlacementName);
                FromJsonUtilP(input["PlacementViewsRemaining"], PlacementViewsRemaining);
                FromJsonUtilP(input["PlacementViewsResetMinutes"], PlacementViewsResetMinutes);
                FromJsonUtilS(input["RewardAssetUrl"], RewardAssetUrl);
                FromJsonUtilS(input["RewardDescription"], RewardDescription);
                FromJsonUtilS(input["RewardId"], RewardId);
                FromJsonUtilS(input["RewardName"], RewardName);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_PlacementId; ToJsonUtilS(PlacementId, each_PlacementId); output["PlacementId"] = each_PlacementId;
                Json::Value each_PlacementName; ToJsonUtilS(PlacementName, each_PlacementName); output["PlacementName"] = each_PlacementName;
                Json::Value each_PlacementViewsRemaining; ToJsonUtilP(PlacementViewsRemaining, each_PlacementViewsRemaining); output["PlacementViewsRemaining"] = each_PlacementViewsRemaining;
                Json::Value each_PlacementViewsResetMinutes; ToJsonUtilP(PlacementViewsResetMinutes, each_PlacementViewsResetMinutes); output["PlacementViewsResetMinutes"] = each_PlacementViewsResetMinutes;
                Json::Value each_RewardAssetUrl; ToJsonUtilS(RewardAssetUrl, each_RewardAssetUrl); output["RewardAssetUrl"] = each_RewardAssetUrl;
                Json::Value each_RewardDescription; ToJsonUtilS(RewardDescription, each_RewardDescription); output["RewardDescription"] = each_RewardDescription;
                Json::Value each_RewardId; ToJsonUtilS(RewardId, each_RewardId); output["RewardId"] = each_RewardId;
                Json::Value each_RewardName; ToJsonUtilS(RewardName, each_RewardName); output["RewardName"] = each_RewardName;
                return output;
            }
        };

        struct AdRewardItemGranted : public PlayFabBaseModel
        {
            std::string CatalogId;
            std::string DisplayName;
            std::string InstanceId;
            std::string ItemId;

            AdRewardItemGranted() :
                PlayFabBaseModel(),
                CatalogId(),
                DisplayName(),
                InstanceId(),
                ItemId()
            {}

            AdRewardItemGranted(const AdRewardItemGranted& src) :
                PlayFabBaseModel(),
                CatalogId(src.CatalogId),
                DisplayName(src.DisplayName),
                InstanceId(src.InstanceId),
                ItemId(src.ItemId)
            {}

            ~AdRewardItemGranted() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["CatalogId"], CatalogId);
                FromJsonUtilS(input["DisplayName"], DisplayName);
                FromJsonUtilS(input["InstanceId"], InstanceId);
                FromJsonUtilS(input["ItemId"], ItemId);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_CatalogId; ToJsonUtilS(CatalogId, each_CatalogId); output["CatalogId"] = each_CatalogId;
                Json::Value each_DisplayName; ToJsonUtilS(DisplayName, each_DisplayName); output["DisplayName"] = each_DisplayName;
                Json::Value each_InstanceId; ToJsonUtilS(InstanceId, each_InstanceId); output["InstanceId"] = each_InstanceId;
                Json::Value each_ItemId; ToJsonUtilS(ItemId, each_ItemId); output["ItemId"] = each_ItemId;
                return output;
            }
        };

        struct AdRewardResults : public PlayFabBaseModel
        {
            std::list<AdRewardItemGranted> GrantedItems;
            std::map<std::string, Int32> GrantedVirtualCurrencies;
            std::map<std::string, Int32> IncrementedStatistics;

            AdRewardResults() :
                PlayFabBaseModel(),
                GrantedItems(),
                GrantedVirtualCurrencies(),
                IncrementedStatistics()
            {}

            AdRewardResults(const AdRewardResults& src) :
                PlayFabBaseModel(),
                GrantedItems(src.GrantedItems),
                GrantedVirtualCurrencies(src.GrantedVirtualCurrencies),
                IncrementedStatistics(src.IncrementedStatistics)
            {}

            ~AdRewardResults() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilO(input["GrantedItems"], GrantedItems);
                FromJsonUtilP(input["GrantedVirtualCurrencies"], GrantedVirtualCurrencies);
                FromJsonUtilP(input["IncrementedStatistics"], IncrementedStatistics);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_GrantedItems; ToJsonUtilO(GrantedItems, each_GrantedItems); output["GrantedItems"] = each_GrantedItems;
                Json::Value each_GrantedVirtualCurrencies; ToJsonUtilP(GrantedVirtualCurrencies, each_GrantedVirtualCurrencies); output["GrantedVirtualCurrencies"] = each_GrantedVirtualCurrencies;
                Json::Value each_IncrementedStatistics; ToJsonUtilP(IncrementedStatistics, each_IncrementedStatistics); output["IncrementedStatistics"] = each_IncrementedStatistics;
                return output;
            }
        };

        struct AndroidDevicePushNotificationRegistrationRequest : public PlayFabRequestCommon
        {
            std::string ConfirmationMessage;
            std::string DeviceToken;
            Boxed<bool> SendPushNotificationConfirmation;

            AndroidDevicePushNotificationRegistrationRequest() :
                PlayFabRequestCommon(),
                ConfirmationMessage(),
                DeviceToken(),
                SendPushNotificationConfirmation()
            {}

            AndroidDevicePushNotificationRegistrationRequest(const AndroidDevicePushNotificationRegistrationRequest& src) :
                PlayFabRequestCommon(),
                ConfirmationMessage(src.ConfirmationMessage),
                DeviceToken(src.DeviceToken),
                SendPushNotificationConfirmation(src.SendPushNotificationConfirmation)
            {}

            ~AndroidDevicePushNotificationRegistrationRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["ConfirmationMessage"], ConfirmationMessage);
                FromJsonUtilS(input["DeviceToken"], DeviceToken);
                FromJsonUtilP(input["SendPushNotificationConfirmation"], SendPushNotificationConfirmation);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_ConfirmationMessage; ToJsonUtilS(ConfirmationMessage, each_ConfirmationMessage); output["ConfirmationMessage"] = each_ConfirmationMessage;
                Json::Value each_DeviceToken; ToJsonUtilS(DeviceToken, each_DeviceToken); output["DeviceToken"] = each_DeviceToken;
                Json::Value each_SendPushNotificationConfirmation; ToJsonUtilP(SendPushNotificationConfirmation, each_SendPushNotificationConfirmation); output["SendPushNotificationConfirmation"] = each_SendPushNotificationConfirmation;
                return output;
            }
        };

        struct AndroidDevicePushNotificationRegistrationResult : public PlayFabResultCommon
        {

            AndroidDevicePushNotificationRegistrationResult() :
                PlayFabResultCommon()
            {}

            AndroidDevicePushNotificationRegistrationResult(const AndroidDevicePushNotificationRegistrationResult&) :
                PlayFabResultCommon()
            {}

            ~AndroidDevicePushNotificationRegistrationResult() = default;

            void FromJson(const Json::Value&) override
            {
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                return output;
            }
        };

        struct AttributeInstallRequest : public PlayFabRequestCommon
        {
            std::string Adid;
            std::string Idfa;

            AttributeInstallRequest() :
                PlayFabRequestCommon(),
                Adid(),
                Idfa()
            {}

            AttributeInstallRequest(const AttributeInstallRequest& src) :
                PlayFabRequestCommon(),
                Adid(src.Adid),
                Idfa(src.Idfa)
            {}

            ~AttributeInstallRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["Adid"], Adid);
                FromJsonUtilS(input["Idfa"], Idfa);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_Adid; ToJsonUtilS(Adid, each_Adid); output["Adid"] = each_Adid;
                Json::Value each_Idfa; ToJsonUtilS(Idfa, each_Idfa); output["Idfa"] = each_Idfa;
                return output;
            }
        };

        struct AttributeInstallResult : public PlayFabResultCommon
        {

            AttributeInstallResult() :
                PlayFabResultCommon()
            {}

            AttributeInstallResult(const AttributeInstallResult&) :
                PlayFabResultCommon()
            {}

            ~AttributeInstallResult() = default;

            void FromJson(const Json::Value&) override
            {
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                return output;
            }
        };

        struct CancelTradeRequest : public PlayFabRequestCommon
        {
            std::string TradeId;

            CancelTradeRequest() :
                PlayFabRequestCommon(),
                TradeId()
            {}

            CancelTradeRequest(const CancelTradeRequest& src) :
                PlayFabRequestCommon(),
                TradeId(src.TradeId)
            {}

            ~CancelTradeRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["TradeId"], TradeId);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_TradeId; ToJsonUtilS(TradeId, each_TradeId); output["TradeId"] = each_TradeId;
                return output;
            }
        };

        struct CancelTradeResponse : public PlayFabResultCommon
        {
            Boxed<TradeInfo> Trade;

            CancelTradeResponse() :
                PlayFabResultCommon(),
                Trade()
            {}

            CancelTradeResponse(const CancelTradeResponse& src) :
                PlayFabResultCommon(),
                Trade(src.Trade)
            {}

            ~CancelTradeResponse() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilO(input["Trade"], Trade);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_Trade; ToJsonUtilO(Trade, each_Trade); output["Trade"] = each_Trade;
                return output;
            }
        };

        struct CartItem : public PlayFabBaseModel
        {
            std::string Description;
            std::string DisplayName;
            std::string ItemClass;
            std::string ItemId;
            std::string ItemInstanceId;
            std::map<std::string, Uint32> RealCurrencyPrices;
            std::map<std::string, Uint32> VCAmount;
            std::map<std::string, Uint32> VirtualCurrencyPrices;

            CartItem() :
                PlayFabBaseModel(),
                Description(),
                DisplayName(),
                ItemClass(),
                ItemId(),
                ItemInstanceId(),
                RealCurrencyPrices(),
                VCAmount(),
                VirtualCurrencyPrices()
            {}

            CartItem(const CartItem& src) :
                PlayFabBaseModel(),
                Description(src.Description),
                DisplayName(src.DisplayName),
                ItemClass(src.ItemClass),
                ItemId(src.ItemId),
                ItemInstanceId(src.ItemInstanceId),
                RealCurrencyPrices(src.RealCurrencyPrices),
                VCAmount(src.VCAmount),
                VirtualCurrencyPrices(src.VirtualCurrencyPrices)
            {}

            ~CartItem() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["Description"], Description);
                FromJsonUtilS(input["DisplayName"], DisplayName);
                FromJsonUtilS(input["ItemClass"], ItemClass);
                FromJsonUtilS(input["ItemId"], ItemId);
                FromJsonUtilS(input["ItemInstanceId"], ItemInstanceId);
                FromJsonUtilP(input["RealCurrencyPrices"], RealCurrencyPrices);
                FromJsonUtilP(input["VCAmount"], VCAmount);
                FromJsonUtilP(input["VirtualCurrencyPrices"], VirtualCurrencyPrices);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_Description; ToJsonUtilS(Description, each_Description); output["Description"] = each_Description;
                Json::Value each_DisplayName; ToJsonUtilS(DisplayName, each_DisplayName); output["DisplayName"] = each_DisplayName;
                Json::Value each_ItemClass; ToJsonUtilS(ItemClass, each_ItemClass); output["ItemClass"] = each_ItemClass;
                Json::Value each_ItemId; ToJsonUtilS(ItemId, each_ItemId); output["ItemId"] = each_ItemId;
                Json::Value each_ItemInstanceId; ToJsonUtilS(ItemInstanceId, each_ItemInstanceId); output["ItemInstanceId"] = each_ItemInstanceId;
                Json::Value each_RealCurrencyPrices; ToJsonUtilP(RealCurrencyPrices, each_RealCurrencyPrices); output["RealCurrencyPrices"] = each_RealCurrencyPrices;
                Json::Value each_VCAmount; ToJsonUtilP(VCAmount, each_VCAmount); output["VCAmount"] = each_VCAmount;
                Json::Value each_VirtualCurrencyPrices; ToJsonUtilP(VirtualCurrencyPrices, each_VirtualCurrencyPrices); output["VirtualCurrencyPrices"] = each_VirtualCurrencyPrices;
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

        struct CharacterInventory : public PlayFabBaseModel
        {
            std::string CharacterId;
            std::list<ItemInstance> Inventory;

            CharacterInventory() :
                PlayFabBaseModel(),
                CharacterId(),
                Inventory()
            {}

            CharacterInventory(const CharacterInventory& src) :
                PlayFabBaseModel(),
                CharacterId(src.CharacterId),
                Inventory(src.Inventory)
            {}

            ~CharacterInventory() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["CharacterId"], CharacterId);
                FromJsonUtilO(input["Inventory"], Inventory);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_CharacterId; ToJsonUtilS(CharacterId, each_CharacterId); output["CharacterId"] = each_CharacterId;
                Json::Value each_Inventory; ToJsonUtilO(Inventory, each_Inventory); output["Inventory"] = each_Inventory;
                return output;
            }
        };

        struct CharacterLeaderboardEntry : public PlayFabBaseModel
        {
            std::string CharacterId;
            std::string CharacterName;
            std::string CharacterType;
            std::string DisplayName;
            std::string PlayFabId;
            Int32 Position;
            Int32 StatValue;

            CharacterLeaderboardEntry() :
                PlayFabBaseModel(),
                CharacterId(),
                CharacterName(),
                CharacterType(),
                DisplayName(),
                PlayFabId(),
                Position(),
                StatValue()
            {}

            CharacterLeaderboardEntry(const CharacterLeaderboardEntry& src) :
                PlayFabBaseModel(),
                CharacterId(src.CharacterId),
                CharacterName(src.CharacterName),
                CharacterType(src.CharacterType),
                DisplayName(src.DisplayName),
                PlayFabId(src.PlayFabId),
                Position(src.Position),
                StatValue(src.StatValue)
            {}

            ~CharacterLeaderboardEntry() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["CharacterId"], CharacterId);
                FromJsonUtilS(input["CharacterName"], CharacterName);
                FromJsonUtilS(input["CharacterType"], CharacterType);
                FromJsonUtilS(input["DisplayName"], DisplayName);
                FromJsonUtilS(input["PlayFabId"], PlayFabId);
                FromJsonUtilP(input["Position"], Position);
                FromJsonUtilP(input["StatValue"], StatValue);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_CharacterId; ToJsonUtilS(CharacterId, each_CharacterId); output["CharacterId"] = each_CharacterId;
                Json::Value each_CharacterName; ToJsonUtilS(CharacterName, each_CharacterName); output["CharacterName"] = each_CharacterName;
                Json::Value each_CharacterType; ToJsonUtilS(CharacterType, each_CharacterType); output["CharacterType"] = each_CharacterType;
                Json::Value each_DisplayName; ToJsonUtilS(DisplayName, each_DisplayName); output["DisplayName"] = each_DisplayName;
                Json::Value each_PlayFabId; ToJsonUtilS(PlayFabId, each_PlayFabId); output["PlayFabId"] = each_PlayFabId;
                Json::Value each_Position; ToJsonUtilP(Position, each_Position); output["Position"] = each_Position;
                Json::Value each_StatValue; ToJsonUtilP(StatValue, each_StatValue); output["StatValue"] = each_StatValue;
                return output;
            }
        };

        struct CharacterResult : public PlayFabResultCommon
        {
            std::string CharacterId;
            std::string CharacterName;
            std::string CharacterType;

            CharacterResult() :
                PlayFabResultCommon(),
                CharacterId(),
                CharacterName(),
                CharacterType()
            {}

            CharacterResult(const CharacterResult& src) :
                PlayFabResultCommon(),
                CharacterId(src.CharacterId),
                CharacterName(src.CharacterName),
                CharacterType(src.CharacterType)
            {}

            ~CharacterResult() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["CharacterId"], CharacterId);
                FromJsonUtilS(input["CharacterName"], CharacterName);
                FromJsonUtilS(input["CharacterType"], CharacterType);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_CharacterId; ToJsonUtilS(CharacterId, each_CharacterId); output["CharacterId"] = each_CharacterId;
                Json::Value each_CharacterName; ToJsonUtilS(CharacterName, each_CharacterName); output["CharacterName"] = each_CharacterName;
                Json::Value each_CharacterType; ToJsonUtilS(CharacterType, each_CharacterType); output["CharacterType"] = each_CharacterType;
                return output;
            }
        };

        struct Container_Dictionary_String_String : public PlayFabBaseModel
        {
            std::map<std::string, std::string> Data;

            Container_Dictionary_String_String() :
                PlayFabBaseModel(),
                Data()
            {}

            Container_Dictionary_String_String(const Container_Dictionary_String_String& src) :
                PlayFabBaseModel(),
                Data(src.Data)
            {}

            ~Container_Dictionary_String_String() = default;

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

        struct CollectionFilter : public PlayFabBaseModel
        {
            std::list<Container_Dictionary_String_String> Excludes;
            std::list<Container_Dictionary_String_String> Includes;

            CollectionFilter() :
                PlayFabBaseModel(),
                Excludes(),
                Includes()
            {}

            CollectionFilter(const CollectionFilter& src) :
                PlayFabBaseModel(),
                Excludes(src.Excludes),
                Includes(src.Includes)
            {}

            ~CollectionFilter() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilO(input["Excludes"], Excludes);
                FromJsonUtilO(input["Includes"], Includes);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_Excludes; ToJsonUtilO(Excludes, each_Excludes); output["Excludes"] = each_Excludes;
                Json::Value each_Includes; ToJsonUtilO(Includes, each_Includes); output["Includes"] = each_Includes;
                return output;
            }
        };

        struct ConfirmPurchaseRequest : public PlayFabRequestCommon
        {
            std::map<std::string, std::string> CustomTags;
            std::string OrderId;

            ConfirmPurchaseRequest() :
                PlayFabRequestCommon(),
                CustomTags(),
                OrderId()
            {}

            ConfirmPurchaseRequest(const ConfirmPurchaseRequest& src) :
                PlayFabRequestCommon(),
                CustomTags(src.CustomTags),
                OrderId(src.OrderId)
            {}

            ~ConfirmPurchaseRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["CustomTags"], CustomTags);
                FromJsonUtilS(input["OrderId"], OrderId);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_CustomTags; ToJsonUtilS(CustomTags, each_CustomTags); output["CustomTags"] = each_CustomTags;
                Json::Value each_OrderId; ToJsonUtilS(OrderId, each_OrderId); output["OrderId"] = each_OrderId;
                return output;
            }
        };

        struct ConfirmPurchaseResult : public PlayFabResultCommon
        {
            std::list<ItemInstance> Items;
            std::string OrderId;
            time_t PurchaseDate;

            ConfirmPurchaseResult() :
                PlayFabResultCommon(),
                Items(),
                OrderId(),
                PurchaseDate()
            {}

            ConfirmPurchaseResult(const ConfirmPurchaseResult& src) :
                PlayFabResultCommon(),
                Items(src.Items),
                OrderId(src.OrderId),
                PurchaseDate(src.PurchaseDate)
            {}

            ~ConfirmPurchaseResult() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilO(input["Items"], Items);
                FromJsonUtilS(input["OrderId"], OrderId);
                FromJsonUtilT(input["PurchaseDate"], PurchaseDate);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_Items; ToJsonUtilO(Items, each_Items); output["Items"] = each_Items;
                Json::Value each_OrderId; ToJsonUtilS(OrderId, each_OrderId); output["OrderId"] = each_OrderId;
                Json::Value each_PurchaseDate; ToJsonUtilT(PurchaseDate, each_PurchaseDate); output["PurchaseDate"] = each_PurchaseDate;
                return output;
            }
        };

        struct ConsumeItemRequest : public PlayFabRequestCommon
        {
            std::string CharacterId;
            Int32 ConsumeCount;
            std::map<std::string, std::string> CustomTags;
            std::string ItemInstanceId;

            ConsumeItemRequest() :
                PlayFabRequestCommon(),
                CharacterId(),
                ConsumeCount(),
                CustomTags(),
                ItemInstanceId()
            {}

            ConsumeItemRequest(const ConsumeItemRequest& src) :
                PlayFabRequestCommon(),
                CharacterId(src.CharacterId),
                ConsumeCount(src.ConsumeCount),
                CustomTags(src.CustomTags),
                ItemInstanceId(src.ItemInstanceId)
            {}

            ~ConsumeItemRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["CharacterId"], CharacterId);
                FromJsonUtilP(input["ConsumeCount"], ConsumeCount);
                FromJsonUtilS(input["CustomTags"], CustomTags);
                FromJsonUtilS(input["ItemInstanceId"], ItemInstanceId);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_CharacterId; ToJsonUtilS(CharacterId, each_CharacterId); output["CharacterId"] = each_CharacterId;
                Json::Value each_ConsumeCount; ToJsonUtilP(ConsumeCount, each_ConsumeCount); output["ConsumeCount"] = each_ConsumeCount;
                Json::Value each_CustomTags; ToJsonUtilS(CustomTags, each_CustomTags); output["CustomTags"] = each_CustomTags;
                Json::Value each_ItemInstanceId; ToJsonUtilS(ItemInstanceId, each_ItemInstanceId); output["ItemInstanceId"] = each_ItemInstanceId;
                return output;
            }
        };

        struct ConsumeItemResult : public PlayFabResultCommon
        {
            std::string ItemInstanceId;
            Int32 RemainingUses;

            ConsumeItemResult() :
                PlayFabResultCommon(),
                ItemInstanceId(),
                RemainingUses()
            {}

            ConsumeItemResult(const ConsumeItemResult& src) :
                PlayFabResultCommon(),
                ItemInstanceId(src.ItemInstanceId),
                RemainingUses(src.RemainingUses)
            {}

            ~ConsumeItemResult() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["ItemInstanceId"], ItemInstanceId);
                FromJsonUtilP(input["RemainingUses"], RemainingUses);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_ItemInstanceId; ToJsonUtilS(ItemInstanceId, each_ItemInstanceId); output["ItemInstanceId"] = each_ItemInstanceId;
                Json::Value each_RemainingUses; ToJsonUtilP(RemainingUses, each_RemainingUses); output["RemainingUses"] = each_RemainingUses;
                return output;
            }
        };

        struct MicrosoftStorePayload : public PlayFabBaseModel
        {
            std::string CollectionsMsIdKey;
            std::string UserId;
            std::string XboxToken;

            MicrosoftStorePayload() :
                PlayFabBaseModel(),
                CollectionsMsIdKey(),
                UserId(),
                XboxToken()
            {}

            MicrosoftStorePayload(const MicrosoftStorePayload& src) :
                PlayFabBaseModel(),
                CollectionsMsIdKey(src.CollectionsMsIdKey),
                UserId(src.UserId),
                XboxToken(src.XboxToken)
            {}

            ~MicrosoftStorePayload() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["CollectionsMsIdKey"], CollectionsMsIdKey);
                FromJsonUtilS(input["UserId"], UserId);
                FromJsonUtilS(input["XboxToken"], XboxToken);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_CollectionsMsIdKey; ToJsonUtilS(CollectionsMsIdKey, each_CollectionsMsIdKey); output["CollectionsMsIdKey"] = each_CollectionsMsIdKey;
                Json::Value each_UserId; ToJsonUtilS(UserId, each_UserId); output["UserId"] = each_UserId;
                Json::Value each_XboxToken; ToJsonUtilS(XboxToken, each_XboxToken); output["XboxToken"] = each_XboxToken;
                return output;
            }
        };

        struct ConsumeMicrosoftStoreEntitlementsRequest : public PlayFabRequestCommon
        {
            std::string CatalogVersion;
            std::map<std::string, std::string> CustomTags;
            MicrosoftStorePayload MarketplaceSpecificData;

            ConsumeMicrosoftStoreEntitlementsRequest() :
                PlayFabRequestCommon(),
                CatalogVersion(),
                CustomTags(),
                MarketplaceSpecificData()
            {}

            ConsumeMicrosoftStoreEntitlementsRequest(const ConsumeMicrosoftStoreEntitlementsRequest& src) :
                PlayFabRequestCommon(),
                CatalogVersion(src.CatalogVersion),
                CustomTags(src.CustomTags),
                MarketplaceSpecificData(src.MarketplaceSpecificData)
            {}

            ~ConsumeMicrosoftStoreEntitlementsRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["CatalogVersion"], CatalogVersion);
                FromJsonUtilS(input["CustomTags"], CustomTags);
                FromJsonUtilO(input["MarketplaceSpecificData"], MarketplaceSpecificData);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_CatalogVersion; ToJsonUtilS(CatalogVersion, each_CatalogVersion); output["CatalogVersion"] = each_CatalogVersion;
                Json::Value each_CustomTags; ToJsonUtilS(CustomTags, each_CustomTags); output["CustomTags"] = each_CustomTags;
                Json::Value each_MarketplaceSpecificData; ToJsonUtilO(MarketplaceSpecificData, each_MarketplaceSpecificData); output["MarketplaceSpecificData"] = each_MarketplaceSpecificData;
                return output;
            }
        };

        struct ConsumeMicrosoftStoreEntitlementsResponse : public PlayFabResultCommon
        {
            std::list<ItemInstance> Items;

            ConsumeMicrosoftStoreEntitlementsResponse() :
                PlayFabResultCommon(),
                Items()
            {}

            ConsumeMicrosoftStoreEntitlementsResponse(const ConsumeMicrosoftStoreEntitlementsResponse& src) :
                PlayFabResultCommon(),
                Items(src.Items)
            {}

            ~ConsumeMicrosoftStoreEntitlementsResponse() = default;

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

        struct PlayStation5Payload : public PlayFabBaseModel
        {
            std::list<std::string> Ids;
            std::string ServiceLabel;

            PlayStation5Payload() :
                PlayFabBaseModel(),
                Ids(),
                ServiceLabel()
            {}

            PlayStation5Payload(const PlayStation5Payload& src) :
                PlayFabBaseModel(),
                Ids(src.Ids),
                ServiceLabel(src.ServiceLabel)
            {}

            ~PlayStation5Payload() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["Ids"], Ids);
                FromJsonUtilS(input["ServiceLabel"], ServiceLabel);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_Ids; ToJsonUtilS(Ids, each_Ids); output["Ids"] = each_Ids;
                Json::Value each_ServiceLabel; ToJsonUtilS(ServiceLabel, each_ServiceLabel); output["ServiceLabel"] = each_ServiceLabel;
                return output;
            }
        };

        struct ConsumePS5EntitlementsRequest : public PlayFabRequestCommon
        {
            std::string CatalogVersion;
            std::map<std::string, std::string> CustomTags;
            PlayStation5Payload MarketplaceSpecificData;

            ConsumePS5EntitlementsRequest() :
                PlayFabRequestCommon(),
                CatalogVersion(),
                CustomTags(),
                MarketplaceSpecificData()
            {}

            ConsumePS5EntitlementsRequest(const ConsumePS5EntitlementsRequest& src) :
                PlayFabRequestCommon(),
                CatalogVersion(src.CatalogVersion),
                CustomTags(src.CustomTags),
                MarketplaceSpecificData(src.MarketplaceSpecificData)
            {}

            ~ConsumePS5EntitlementsRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["CatalogVersion"], CatalogVersion);
                FromJsonUtilS(input["CustomTags"], CustomTags);
                FromJsonUtilO(input["MarketplaceSpecificData"], MarketplaceSpecificData);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_CatalogVersion; ToJsonUtilS(CatalogVersion, each_CatalogVersion); output["CatalogVersion"] = each_CatalogVersion;
                Json::Value each_CustomTags; ToJsonUtilS(CustomTags, each_CustomTags); output["CustomTags"] = each_CustomTags;
                Json::Value each_MarketplaceSpecificData; ToJsonUtilO(MarketplaceSpecificData, each_MarketplaceSpecificData); output["MarketplaceSpecificData"] = each_MarketplaceSpecificData;
                return output;
            }
        };

        struct ConsumePS5EntitlementsResult : public PlayFabResultCommon
        {
            std::list<ItemInstance> Items;

            ConsumePS5EntitlementsResult() :
                PlayFabResultCommon(),
                Items()
            {}

            ConsumePS5EntitlementsResult(const ConsumePS5EntitlementsResult& src) :
                PlayFabResultCommon(),
                Items(src.Items)
            {}

            ~ConsumePS5EntitlementsResult() = default;

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

        struct ConsumePSNEntitlementsRequest : public PlayFabRequestCommon
        {
            std::string CatalogVersion;
            std::map<std::string, std::string> CustomTags;
            Int32 ServiceLabel;

            ConsumePSNEntitlementsRequest() :
                PlayFabRequestCommon(),
                CatalogVersion(),
                CustomTags(),
                ServiceLabel()
            {}

            ConsumePSNEntitlementsRequest(const ConsumePSNEntitlementsRequest& src) :
                PlayFabRequestCommon(),
                CatalogVersion(src.CatalogVersion),
                CustomTags(src.CustomTags),
                ServiceLabel(src.ServiceLabel)
            {}

            ~ConsumePSNEntitlementsRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["CatalogVersion"], CatalogVersion);
                FromJsonUtilS(input["CustomTags"], CustomTags);
                FromJsonUtilP(input["ServiceLabel"], ServiceLabel);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_CatalogVersion; ToJsonUtilS(CatalogVersion, each_CatalogVersion); output["CatalogVersion"] = each_CatalogVersion;
                Json::Value each_CustomTags; ToJsonUtilS(CustomTags, each_CustomTags); output["CustomTags"] = each_CustomTags;
                Json::Value each_ServiceLabel; ToJsonUtilP(ServiceLabel, each_ServiceLabel); output["ServiceLabel"] = each_ServiceLabel;
                return output;
            }
        };

        struct ConsumePSNEntitlementsResult : public PlayFabResultCommon
        {
            std::list<ItemInstance> ItemsGranted;

            ConsumePSNEntitlementsResult() :
                PlayFabResultCommon(),
                ItemsGranted()
            {}

            ConsumePSNEntitlementsResult(const ConsumePSNEntitlementsResult& src) :
                PlayFabResultCommon(),
                ItemsGranted(src.ItemsGranted)
            {}

            ~ConsumePSNEntitlementsResult() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilO(input["ItemsGranted"], ItemsGranted);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_ItemsGranted; ToJsonUtilO(ItemsGranted, each_ItemsGranted); output["ItemsGranted"] = each_ItemsGranted;
                return output;
            }
        };

        struct ConsumeXboxEntitlementsRequest : public PlayFabRequestCommon
        {
            std::string CatalogVersion;
            std::map<std::string, std::string> CustomTags;
            std::string XboxToken;

            ConsumeXboxEntitlementsRequest() :
                PlayFabRequestCommon(),
                CatalogVersion(),
                CustomTags(),
                XboxToken()
            {}

            ConsumeXboxEntitlementsRequest(const ConsumeXboxEntitlementsRequest& src) :
                PlayFabRequestCommon(),
                CatalogVersion(src.CatalogVersion),
                CustomTags(src.CustomTags),
                XboxToken(src.XboxToken)
            {}

            ~ConsumeXboxEntitlementsRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["CatalogVersion"], CatalogVersion);
                FromJsonUtilS(input["CustomTags"], CustomTags);
                FromJsonUtilS(input["XboxToken"], XboxToken);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_CatalogVersion; ToJsonUtilS(CatalogVersion, each_CatalogVersion); output["CatalogVersion"] = each_CatalogVersion;
                Json::Value each_CustomTags; ToJsonUtilS(CustomTags, each_CustomTags); output["CustomTags"] = each_CustomTags;
                Json::Value each_XboxToken; ToJsonUtilS(XboxToken, each_XboxToken); output["XboxToken"] = each_XboxToken;
                return output;
            }
        };

        struct ConsumeXboxEntitlementsResult : public PlayFabResultCommon
        {
            std::list<ItemInstance> Items;

            ConsumeXboxEntitlementsResult() :
                PlayFabResultCommon(),
                Items()
            {}

            ConsumeXboxEntitlementsResult(const ConsumeXboxEntitlementsResult& src) :
                PlayFabResultCommon(),
                Items(src.Items)
            {}

            ~ConsumeXboxEntitlementsResult() = default;

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

        struct CreateSharedGroupRequest : public PlayFabRequestCommon
        {
            std::string SharedGroupId;

            CreateSharedGroupRequest() :
                PlayFabRequestCommon(),
                SharedGroupId()
            {}

            CreateSharedGroupRequest(const CreateSharedGroupRequest& src) :
                PlayFabRequestCommon(),
                SharedGroupId(src.SharedGroupId)
            {}

            ~CreateSharedGroupRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["SharedGroupId"], SharedGroupId);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_SharedGroupId; ToJsonUtilS(SharedGroupId, each_SharedGroupId); output["SharedGroupId"] = each_SharedGroupId;
                return output;
            }
        };

        struct CreateSharedGroupResult : public PlayFabResultCommon
        {
            std::string SharedGroupId;

            CreateSharedGroupResult() :
                PlayFabResultCommon(),
                SharedGroupId()
            {}

            CreateSharedGroupResult(const CreateSharedGroupResult& src) :
                PlayFabResultCommon(),
                SharedGroupId(src.SharedGroupId)
            {}

            ~CreateSharedGroupResult() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["SharedGroupId"], SharedGroupId);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_SharedGroupId; ToJsonUtilS(SharedGroupId, each_SharedGroupId); output["SharedGroupId"] = each_SharedGroupId;
                return output;
            }
        };

        struct CurrentGamesRequest : public PlayFabRequestCommon
        {
            std::string BuildVersion;
            std::string GameMode;
            Boxed<Region> pfRegion;
            std::string StatisticName;
            Boxed<CollectionFilter> TagFilter;

            CurrentGamesRequest() :
                PlayFabRequestCommon(),
                BuildVersion(),
                GameMode(),
                pfRegion(),
                StatisticName(),
                TagFilter()
            {}

            CurrentGamesRequest(const CurrentGamesRequest& src) :
                PlayFabRequestCommon(),
                BuildVersion(src.BuildVersion),
                GameMode(src.GameMode),
                pfRegion(src.pfRegion),
                StatisticName(src.StatisticName),
                TagFilter(src.TagFilter)
            {}

            ~CurrentGamesRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["BuildVersion"], BuildVersion);
                FromJsonUtilS(input["GameMode"], GameMode);
                FromJsonUtilE(input["Region"], pfRegion);
                FromJsonUtilS(input["StatisticName"], StatisticName);
                FromJsonUtilO(input["TagFilter"], TagFilter);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_BuildVersion; ToJsonUtilS(BuildVersion, each_BuildVersion); output["BuildVersion"] = each_BuildVersion;
                Json::Value each_GameMode; ToJsonUtilS(GameMode, each_GameMode); output["GameMode"] = each_GameMode;
                Json::Value each_pfRegion; ToJsonUtilE(pfRegion, each_pfRegion); output["Region"] = each_pfRegion;
                Json::Value each_StatisticName; ToJsonUtilS(StatisticName, each_StatisticName); output["StatisticName"] = each_StatisticName;
                Json::Value each_TagFilter; ToJsonUtilO(TagFilter, each_TagFilter); output["TagFilter"] = each_TagFilter;
                return output;
            }
        };

        struct GameInfo : public PlayFabBaseModel
        {
            std::string BuildVersion;
            std::string GameMode;
            std::string GameServerData;
            Boxed<GameInstanceState> GameServerStateEnum;
            Boxed<time_t> LastHeartbeat;
            std::string LobbyID;
            Boxed<Int32> MaxPlayers;
            std::list<std::string> PlayerUserIds;
            Boxed<Region> pfRegion;
            Uint32 RunTime;
            std::string ServerIPV4Address;
            std::string ServerIPV6Address;
            Boxed<Int32> ServerPort;
            std::string ServerPublicDNSName;
            std::string StatisticName;
            std::map<std::string, std::string> Tags;

            GameInfo() :
                PlayFabBaseModel(),
                BuildVersion(),
                GameMode(),
                GameServerData(),
                GameServerStateEnum(),
                LastHeartbeat(),
                LobbyID(),
                MaxPlayers(),
                PlayerUserIds(),
                pfRegion(),
                RunTime(),
                ServerIPV4Address(),
                ServerIPV6Address(),
                ServerPort(),
                ServerPublicDNSName(),
                StatisticName(),
                Tags()
            {}

            GameInfo(const GameInfo& src) :
                PlayFabBaseModel(),
                BuildVersion(src.BuildVersion),
                GameMode(src.GameMode),
                GameServerData(src.GameServerData),
                GameServerStateEnum(src.GameServerStateEnum),
                LastHeartbeat(src.LastHeartbeat),
                LobbyID(src.LobbyID),
                MaxPlayers(src.MaxPlayers),
                PlayerUserIds(src.PlayerUserIds),
                pfRegion(src.pfRegion),
                RunTime(src.RunTime),
                ServerIPV4Address(src.ServerIPV4Address),
                ServerIPV6Address(src.ServerIPV6Address),
                ServerPort(src.ServerPort),
                ServerPublicDNSName(src.ServerPublicDNSName),
                StatisticName(src.StatisticName),
                Tags(src.Tags)
            {}

            ~GameInfo() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["BuildVersion"], BuildVersion);
                FromJsonUtilS(input["GameMode"], GameMode);
                FromJsonUtilS(input["GameServerData"], GameServerData);
                FromJsonUtilE(input["GameServerStateEnum"], GameServerStateEnum);
                FromJsonUtilT(input["LastHeartbeat"], LastHeartbeat);
                FromJsonUtilS(input["LobbyID"], LobbyID);
                FromJsonUtilP(input["MaxPlayers"], MaxPlayers);
                FromJsonUtilS(input["PlayerUserIds"], PlayerUserIds);
                FromJsonUtilE(input["Region"], pfRegion);
                FromJsonUtilP(input["RunTime"], RunTime);
                FromJsonUtilS(input["ServerIPV4Address"], ServerIPV4Address);
                FromJsonUtilS(input["ServerIPV6Address"], ServerIPV6Address);
                FromJsonUtilP(input["ServerPort"], ServerPort);
                FromJsonUtilS(input["ServerPublicDNSName"], ServerPublicDNSName);
                FromJsonUtilS(input["StatisticName"], StatisticName);
                FromJsonUtilS(input["Tags"], Tags);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_BuildVersion; ToJsonUtilS(BuildVersion, each_BuildVersion); output["BuildVersion"] = each_BuildVersion;
                Json::Value each_GameMode; ToJsonUtilS(GameMode, each_GameMode); output["GameMode"] = each_GameMode;
                Json::Value each_GameServerData; ToJsonUtilS(GameServerData, each_GameServerData); output["GameServerData"] = each_GameServerData;
                Json::Value each_GameServerStateEnum; ToJsonUtilE(GameServerStateEnum, each_GameServerStateEnum); output["GameServerStateEnum"] = each_GameServerStateEnum;
                Json::Value each_LastHeartbeat; ToJsonUtilT(LastHeartbeat, each_LastHeartbeat); output["LastHeartbeat"] = each_LastHeartbeat;
                Json::Value each_LobbyID; ToJsonUtilS(LobbyID, each_LobbyID); output["LobbyID"] = each_LobbyID;
                Json::Value each_MaxPlayers; ToJsonUtilP(MaxPlayers, each_MaxPlayers); output["MaxPlayers"] = each_MaxPlayers;
                Json::Value each_PlayerUserIds; ToJsonUtilS(PlayerUserIds, each_PlayerUserIds); output["PlayerUserIds"] = each_PlayerUserIds;
                Json::Value each_pfRegion; ToJsonUtilE(pfRegion, each_pfRegion); output["Region"] = each_pfRegion;
                Json::Value each_RunTime; ToJsonUtilP(RunTime, each_RunTime); output["RunTime"] = each_RunTime;
                Json::Value each_ServerIPV4Address; ToJsonUtilS(ServerIPV4Address, each_ServerIPV4Address); output["ServerIPV4Address"] = each_ServerIPV4Address;
                Json::Value each_ServerIPV6Address; ToJsonUtilS(ServerIPV6Address, each_ServerIPV6Address); output["ServerIPV6Address"] = each_ServerIPV6Address;
                Json::Value each_ServerPort; ToJsonUtilP(ServerPort, each_ServerPort); output["ServerPort"] = each_ServerPort;
                Json::Value each_ServerPublicDNSName; ToJsonUtilS(ServerPublicDNSName, each_ServerPublicDNSName); output["ServerPublicDNSName"] = each_ServerPublicDNSName;
                Json::Value each_StatisticName; ToJsonUtilS(StatisticName, each_StatisticName); output["StatisticName"] = each_StatisticName;
                Json::Value each_Tags; ToJsonUtilS(Tags, each_Tags); output["Tags"] = each_Tags;
                return output;
            }
        };

        struct CurrentGamesResult : public PlayFabResultCommon
        {
            Int32 GameCount;
            std::list<GameInfo> Games;
            Int32 PlayerCount;

            CurrentGamesResult() :
                PlayFabResultCommon(),
                GameCount(),
                Games(),
                PlayerCount()
            {}

            CurrentGamesResult(const CurrentGamesResult& src) :
                PlayFabResultCommon(),
                GameCount(src.GameCount),
                Games(src.Games),
                PlayerCount(src.PlayerCount)
            {}

            ~CurrentGamesResult() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilP(input["GameCount"], GameCount);
                FromJsonUtilO(input["Games"], Games);
                FromJsonUtilP(input["PlayerCount"], PlayerCount);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_GameCount; ToJsonUtilP(GameCount, each_GameCount); output["GameCount"] = each_GameCount;
                Json::Value each_Games; ToJsonUtilO(Games, each_Games); output["Games"] = each_Games;
                Json::Value each_PlayerCount; ToJsonUtilP(PlayerCount, each_PlayerCount); output["PlayerCount"] = each_PlayerCount;
                return output;
            }
        };

        struct DeviceInfoRequest : public PlayFabRequestCommon
        {
            Json::Value Info; // Not truly arbitrary. See documentation for restrictions on format

            DeviceInfoRequest() :
                PlayFabRequestCommon(),
                Info()
            {}

            DeviceInfoRequest(const DeviceInfoRequest& src) :
                PlayFabRequestCommon(),
                Info(src.Info)
            {}

            ~DeviceInfoRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                Info = input["Info"];
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                output["Info"] = Info;
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

        struct EntityTokenResponse : public PlayFabResultCommon
        {
            Boxed<EntityKey> Entity;
            std::string EntityToken;
            Boxed<time_t> TokenExpiration;

            EntityTokenResponse() :
                PlayFabResultCommon(),
                Entity(),
                EntityToken(),
                TokenExpiration()
            {}

            EntityTokenResponse(const EntityTokenResponse& src) :
                PlayFabResultCommon(),
                Entity(src.Entity),
                EntityToken(src.EntityToken),
                TokenExpiration(src.TokenExpiration)
            {}

            ~EntityTokenResponse() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilO(input["Entity"], Entity);
                FromJsonUtilS(input["EntityToken"], EntityToken);
                FromJsonUtilT(input["TokenExpiration"], TokenExpiration);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_Entity; ToJsonUtilO(Entity, each_Entity); output["Entity"] = each_Entity;
                Json::Value each_EntityToken; ToJsonUtilS(EntityToken, each_EntityToken); output["EntityToken"] = each_EntityToken;
                Json::Value each_TokenExpiration; ToJsonUtilT(TokenExpiration, each_TokenExpiration); output["TokenExpiration"] = each_TokenExpiration;
                return output;
            }
        };

        struct ExecuteCloudScriptRequest : public PlayFabRequestCommon
        {
            std::map<std::string, std::string> CustomTags;
            std::string FunctionName;
            Json::Value FunctionParameter;
            Boxed<bool> GeneratePlayStreamEvent;
            Boxed<CloudScriptRevisionOption> RevisionSelection;
            Boxed<Int32> SpecificRevision;

            ExecuteCloudScriptRequest() :
                PlayFabRequestCommon(),
                CustomTags(),
                FunctionName(),
                FunctionParameter(),
                GeneratePlayStreamEvent(),
                RevisionSelection(),
                SpecificRevision()
            {}

            ExecuteCloudScriptRequest(const ExecuteCloudScriptRequest& src) :
                PlayFabRequestCommon(),
                CustomTags(src.CustomTags),
                FunctionName(src.FunctionName),
                FunctionParameter(src.FunctionParameter),
                GeneratePlayStreamEvent(src.GeneratePlayStreamEvent),
                RevisionSelection(src.RevisionSelection),
                SpecificRevision(src.SpecificRevision)
            {}

            ~ExecuteCloudScriptRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["CustomTags"], CustomTags);
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
                Json::Value each_FunctionName; ToJsonUtilS(FunctionName, each_FunctionName); output["FunctionName"] = each_FunctionName;
                output["FunctionParameter"] = FunctionParameter;
                Json::Value each_GeneratePlayStreamEvent; ToJsonUtilP(GeneratePlayStreamEvent, each_GeneratePlayStreamEvent); output["GeneratePlayStreamEvent"] = each_GeneratePlayStreamEvent;
                Json::Value each_RevisionSelection; ToJsonUtilE(RevisionSelection, each_RevisionSelection); output["RevisionSelection"] = each_RevisionSelection;
                Json::Value each_SpecificRevision; ToJsonUtilP(SpecificRevision, each_SpecificRevision); output["SpecificRevision"] = each_SpecificRevision;
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

        struct FacebookInstantGamesPlayFabIdPair : public PlayFabBaseModel
        {
            std::string FacebookInstantGamesId;
            std::string PlayFabId;

            FacebookInstantGamesPlayFabIdPair() :
                PlayFabBaseModel(),
                FacebookInstantGamesId(),
                PlayFabId()
            {}

            FacebookInstantGamesPlayFabIdPair(const FacebookInstantGamesPlayFabIdPair& src) :
                PlayFabBaseModel(),
                FacebookInstantGamesId(src.FacebookInstantGamesId),
                PlayFabId(src.PlayFabId)
            {}

            ~FacebookInstantGamesPlayFabIdPair() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["FacebookInstantGamesId"], FacebookInstantGamesId);
                FromJsonUtilS(input["PlayFabId"], PlayFabId);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_FacebookInstantGamesId; ToJsonUtilS(FacebookInstantGamesId, each_FacebookInstantGamesId); output["FacebookInstantGamesId"] = each_FacebookInstantGamesId;
                Json::Value each_PlayFabId; ToJsonUtilS(PlayFabId, each_PlayFabId); output["PlayFabId"] = each_PlayFabId;
                return output;
            }
        };

        struct FacebookPlayFabIdPair : public PlayFabBaseModel
        {
            std::string FacebookId;
            std::string PlayFabId;

            FacebookPlayFabIdPair() :
                PlayFabBaseModel(),
                FacebookId(),
                PlayFabId()
            {}

            FacebookPlayFabIdPair(const FacebookPlayFabIdPair& src) :
                PlayFabBaseModel(),
                FacebookId(src.FacebookId),
                PlayFabId(src.PlayFabId)
            {}

            ~FacebookPlayFabIdPair() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["FacebookId"], FacebookId);
                FromJsonUtilS(input["PlayFabId"], PlayFabId);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_FacebookId; ToJsonUtilS(FacebookId, each_FacebookId); output["FacebookId"] = each_FacebookId;
                Json::Value each_PlayFabId; ToJsonUtilS(PlayFabId, each_PlayFabId); output["PlayFabId"] = each_PlayFabId;
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

        struct FriendInfo : public PlayFabBaseModel
        {
            Boxed<UserFacebookInfo> FacebookInfo;
            std::string FriendPlayFabId;
            Boxed<UserGameCenterInfo> GameCenterInfo;
            Boxed<PlayerProfileModel> Profile;
            Boxed<UserPsnInfo> PSNInfo;
            Boxed<UserSteamInfo> SteamInfo;
            std::list<std::string> Tags;
            std::string TitleDisplayName;
            std::string Username;
            Boxed<UserXboxInfo> XboxInfo;

            FriendInfo() :
                PlayFabBaseModel(),
                FacebookInfo(),
                FriendPlayFabId(),
                GameCenterInfo(),
                Profile(),
                PSNInfo(),
                SteamInfo(),
                Tags(),
                TitleDisplayName(),
                Username(),
                XboxInfo()
            {}

            FriendInfo(const FriendInfo& src) :
                PlayFabBaseModel(),
                FacebookInfo(src.FacebookInfo),
                FriendPlayFabId(src.FriendPlayFabId),
                GameCenterInfo(src.GameCenterInfo),
                Profile(src.Profile),
                PSNInfo(src.PSNInfo),
                SteamInfo(src.SteamInfo),
                Tags(src.Tags),
                TitleDisplayName(src.TitleDisplayName),
                Username(src.Username),
                XboxInfo(src.XboxInfo)
            {}

            ~FriendInfo() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilO(input["FacebookInfo"], FacebookInfo);
                FromJsonUtilS(input["FriendPlayFabId"], FriendPlayFabId);
                FromJsonUtilO(input["GameCenterInfo"], GameCenterInfo);
                FromJsonUtilO(input["Profile"], Profile);
                FromJsonUtilO(input["PSNInfo"], PSNInfo);
                FromJsonUtilO(input["SteamInfo"], SteamInfo);
                FromJsonUtilS(input["Tags"], Tags);
                FromJsonUtilS(input["TitleDisplayName"], TitleDisplayName);
                FromJsonUtilS(input["Username"], Username);
                FromJsonUtilO(input["XboxInfo"], XboxInfo);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_FacebookInfo; ToJsonUtilO(FacebookInfo, each_FacebookInfo); output["FacebookInfo"] = each_FacebookInfo;
                Json::Value each_FriendPlayFabId; ToJsonUtilS(FriendPlayFabId, each_FriendPlayFabId); output["FriendPlayFabId"] = each_FriendPlayFabId;
                Json::Value each_GameCenterInfo; ToJsonUtilO(GameCenterInfo, each_GameCenterInfo); output["GameCenterInfo"] = each_GameCenterInfo;
                Json::Value each_Profile; ToJsonUtilO(Profile, each_Profile); output["Profile"] = each_Profile;
                Json::Value each_PSNInfo; ToJsonUtilO(PSNInfo, each_PSNInfo); output["PSNInfo"] = each_PSNInfo;
                Json::Value each_SteamInfo; ToJsonUtilO(SteamInfo, each_SteamInfo); output["SteamInfo"] = each_SteamInfo;
                Json::Value each_Tags; ToJsonUtilS(Tags, each_Tags); output["Tags"] = each_Tags;
                Json::Value each_TitleDisplayName; ToJsonUtilS(TitleDisplayName, each_TitleDisplayName); output["TitleDisplayName"] = each_TitleDisplayName;
                Json::Value each_Username; ToJsonUtilS(Username, each_Username); output["Username"] = each_Username;
                Json::Value each_XboxInfo; ToJsonUtilO(XboxInfo, each_XboxInfo); output["XboxInfo"] = each_XboxInfo;
                return output;
            }
        };

        struct GameCenterPlayFabIdPair : public PlayFabBaseModel
        {
            std::string GameCenterId;
            std::string PlayFabId;

            GameCenterPlayFabIdPair() :
                PlayFabBaseModel(),
                GameCenterId(),
                PlayFabId()
            {}

            GameCenterPlayFabIdPair(const GameCenterPlayFabIdPair& src) :
                PlayFabBaseModel(),
                GameCenterId(src.GameCenterId),
                PlayFabId(src.PlayFabId)
            {}

            ~GameCenterPlayFabIdPair() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["GameCenterId"], GameCenterId);
                FromJsonUtilS(input["PlayFabId"], PlayFabId);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_GameCenterId; ToJsonUtilS(GameCenterId, each_GameCenterId); output["GameCenterId"] = each_GameCenterId;
                Json::Value each_PlayFabId; ToJsonUtilS(PlayFabId, each_PlayFabId); output["PlayFabId"] = each_PlayFabId;
                return output;
            }
        };

        struct GameServerRegionsRequest : public PlayFabRequestCommon
        {
            std::string BuildVersion;
            std::string TitleId;

            GameServerRegionsRequest() :
                PlayFabRequestCommon(),
                BuildVersion(),
                TitleId()
            {}

            GameServerRegionsRequest(const GameServerRegionsRequest& src) :
                PlayFabRequestCommon(),
                BuildVersion(src.BuildVersion),
                TitleId(src.TitleId)
            {}

            ~GameServerRegionsRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["BuildVersion"], BuildVersion);
                FromJsonUtilS(input["TitleId"], TitleId);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_BuildVersion; ToJsonUtilS(BuildVersion, each_BuildVersion); output["BuildVersion"] = each_BuildVersion;
                Json::Value each_TitleId; ToJsonUtilS(TitleId, each_TitleId); output["TitleId"] = each_TitleId;
                return output;
            }
        };

        struct RegionInfo : public PlayFabBaseModel
        {
            bool Available;
            std::string Name;
            std::string PingUrl;
            Boxed<Region> pfRegion;

            RegionInfo() :
                PlayFabBaseModel(),
                Available(),
                Name(),
                PingUrl(),
                pfRegion()
            {}

            RegionInfo(const RegionInfo& src) :
                PlayFabBaseModel(),
                Available(src.Available),
                Name(src.Name),
                PingUrl(src.PingUrl),
                pfRegion(src.pfRegion)
            {}

            ~RegionInfo() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilP(input["Available"], Available);
                FromJsonUtilS(input["Name"], Name);
                FromJsonUtilS(input["PingUrl"], PingUrl);
                FromJsonUtilE(input["Region"], pfRegion);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_Available; ToJsonUtilP(Available, each_Available); output["Available"] = each_Available;
                Json::Value each_Name; ToJsonUtilS(Name, each_Name); output["Name"] = each_Name;
                Json::Value each_PingUrl; ToJsonUtilS(PingUrl, each_PingUrl); output["PingUrl"] = each_PingUrl;
                Json::Value each_pfRegion; ToJsonUtilE(pfRegion, each_pfRegion); output["Region"] = each_pfRegion;
                return output;
            }
        };

        struct GameServerRegionsResult : public PlayFabResultCommon
        {
            std::list<RegionInfo> Regions;

            GameServerRegionsResult() :
                PlayFabResultCommon(),
                Regions()
            {}

            GameServerRegionsResult(const GameServerRegionsResult& src) :
                PlayFabResultCommon(),
                Regions(src.Regions)
            {}

            ~GameServerRegionsResult() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilO(input["Regions"], Regions);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_Regions; ToJsonUtilO(Regions, each_Regions); output["Regions"] = each_Regions;
                return output;
            }
        };

        struct GenericPlayFabIdPair : public PlayFabBaseModel
        {
            Boxed<GenericServiceId> GenericId;
            std::string PlayFabId;

            GenericPlayFabIdPair() :
                PlayFabBaseModel(),
                GenericId(),
                PlayFabId()
            {}

            GenericPlayFabIdPair(const GenericPlayFabIdPair& src) :
                PlayFabBaseModel(),
                GenericId(src.GenericId),
                PlayFabId(src.PlayFabId)
            {}

            ~GenericPlayFabIdPair() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilO(input["GenericId"], GenericId);
                FromJsonUtilS(input["PlayFabId"], PlayFabId);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_GenericId; ToJsonUtilO(GenericId, each_GenericId); output["GenericId"] = each_GenericId;
                Json::Value each_PlayFabId; ToJsonUtilS(PlayFabId, each_PlayFabId); output["PlayFabId"] = each_PlayFabId;
                return output;
            }
        };

        struct GetAccountInfoRequest : public PlayFabRequestCommon
        {
            std::string Email;
            std::string PlayFabId;
            std::string TitleDisplayName;
            std::string Username;

            GetAccountInfoRequest() :
                PlayFabRequestCommon(),
                Email(),
                PlayFabId(),
                TitleDisplayName(),
                Username()
            {}

            GetAccountInfoRequest(const GetAccountInfoRequest& src) :
                PlayFabRequestCommon(),
                Email(src.Email),
                PlayFabId(src.PlayFabId),
                TitleDisplayName(src.TitleDisplayName),
                Username(src.Username)
            {}

            ~GetAccountInfoRequest() = default;

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
                Json::Value each_XboxInfo; ToJsonUtilO(XboxInfo, each_XboxInfo); output["XboxInfo"] = each_XboxInfo;
                return output;
            }
        };

        struct GetAccountInfoResult : public PlayFabResultCommon
        {
            Boxed<UserAccountInfo> AccountInfo;

            GetAccountInfoResult() :
                PlayFabResultCommon(),
                AccountInfo()
            {}

            GetAccountInfoResult(const GetAccountInfoResult& src) :
                PlayFabResultCommon(),
                AccountInfo(src.AccountInfo)
            {}

            ~GetAccountInfoResult() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilO(input["AccountInfo"], AccountInfo);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_AccountInfo; ToJsonUtilO(AccountInfo, each_AccountInfo); output["AccountInfo"] = each_AccountInfo;
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

        struct GetAdPlacementsRequest : public PlayFabRequestCommon
        {
            std::string AppId;
            Boxed<NameIdentifier> Identifier;

            GetAdPlacementsRequest() :
                PlayFabRequestCommon(),
                AppId(),
                Identifier()
            {}

            GetAdPlacementsRequest(const GetAdPlacementsRequest& src) :
                PlayFabRequestCommon(),
                AppId(src.AppId),
                Identifier(src.Identifier)
            {}

            ~GetAdPlacementsRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["AppId"], AppId);
                FromJsonUtilO(input["Identifier"], Identifier);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_AppId; ToJsonUtilS(AppId, each_AppId); output["AppId"] = each_AppId;
                Json::Value each_Identifier; ToJsonUtilO(Identifier, each_Identifier); output["Identifier"] = each_Identifier;
                return output;
            }
        };

        struct GetAdPlacementsResult : public PlayFabResultCommon
        {
            std::list<AdPlacementDetails> AdPlacements;

            GetAdPlacementsResult() :
                PlayFabResultCommon(),
                AdPlacements()
            {}

            GetAdPlacementsResult(const GetAdPlacementsResult& src) :
                PlayFabResultCommon(),
                AdPlacements(src.AdPlacements)
            {}

            ~GetAdPlacementsResult() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilO(input["AdPlacements"], AdPlacements);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_AdPlacements; ToJsonUtilO(AdPlacements, each_AdPlacements); output["AdPlacements"] = each_AdPlacements;
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

        struct GetCharacterDataRequest : public PlayFabRequestCommon
        {
            std::string CharacterId;
            Boxed<Uint32> IfChangedFromDataVersion;
            std::list<std::string> Keys;
            std::string PlayFabId;

            GetCharacterDataRequest() :
                PlayFabRequestCommon(),
                CharacterId(),
                IfChangedFromDataVersion(),
                Keys(),
                PlayFabId()
            {}

            GetCharacterDataRequest(const GetCharacterDataRequest& src) :
                PlayFabRequestCommon(),
                CharacterId(src.CharacterId),
                IfChangedFromDataVersion(src.IfChangedFromDataVersion),
                Keys(src.Keys),
                PlayFabId(src.PlayFabId)
            {}

            ~GetCharacterDataRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["CharacterId"], CharacterId);
                FromJsonUtilP(input["IfChangedFromDataVersion"], IfChangedFromDataVersion);
                FromJsonUtilS(input["Keys"], Keys);
                FromJsonUtilS(input["PlayFabId"], PlayFabId);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_CharacterId; ToJsonUtilS(CharacterId, each_CharacterId); output["CharacterId"] = each_CharacterId;
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

        struct GetCharacterDataResult : public PlayFabResultCommon
        {
            std::string CharacterId;
            std::map<std::string, UserDataRecord> Data;
            Uint32 DataVersion;

            GetCharacterDataResult() :
                PlayFabResultCommon(),
                CharacterId(),
                Data(),
                DataVersion()
            {}

            GetCharacterDataResult(const GetCharacterDataResult& src) :
                PlayFabResultCommon(),
                CharacterId(src.CharacterId),
                Data(src.Data),
                DataVersion(src.DataVersion)
            {}

            ~GetCharacterDataResult() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["CharacterId"], CharacterId);
                FromJsonUtilO(input["Data"], Data);
                FromJsonUtilP(input["DataVersion"], DataVersion);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_CharacterId; ToJsonUtilS(CharacterId, each_CharacterId); output["CharacterId"] = each_CharacterId;
                Json::Value each_Data; ToJsonUtilO(Data, each_Data); output["Data"] = each_Data;
                Json::Value each_DataVersion; ToJsonUtilP(DataVersion, each_DataVersion); output["DataVersion"] = each_DataVersion;
                return output;
            }
        };

        struct GetCharacterInventoryRequest : public PlayFabRequestCommon
        {
            std::string CatalogVersion;
            std::string CharacterId;
            std::map<std::string, std::string> CustomTags;

            GetCharacterInventoryRequest() :
                PlayFabRequestCommon(),
                CatalogVersion(),
                CharacterId(),
                CustomTags()
            {}

            GetCharacterInventoryRequest(const GetCharacterInventoryRequest& src) :
                PlayFabRequestCommon(),
                CatalogVersion(src.CatalogVersion),
                CharacterId(src.CharacterId),
                CustomTags(src.CustomTags)
            {}

            ~GetCharacterInventoryRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["CatalogVersion"], CatalogVersion);
                FromJsonUtilS(input["CharacterId"], CharacterId);
                FromJsonUtilS(input["CustomTags"], CustomTags);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_CatalogVersion; ToJsonUtilS(CatalogVersion, each_CatalogVersion); output["CatalogVersion"] = each_CatalogVersion;
                Json::Value each_CharacterId; ToJsonUtilS(CharacterId, each_CharacterId); output["CharacterId"] = each_CharacterId;
                Json::Value each_CustomTags; ToJsonUtilS(CustomTags, each_CustomTags); output["CustomTags"] = each_CustomTags;
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

        struct GetCharacterInventoryResult : public PlayFabResultCommon
        {
            std::string CharacterId;
            std::list<ItemInstance> Inventory;
            std::map<std::string, Int32> VirtualCurrency;
            std::map<std::string, VirtualCurrencyRechargeTime> VirtualCurrencyRechargeTimes;

            GetCharacterInventoryResult() :
                PlayFabResultCommon(),
                CharacterId(),
                Inventory(),
                VirtualCurrency(),
                VirtualCurrencyRechargeTimes()
            {}

            GetCharacterInventoryResult(const GetCharacterInventoryResult& src) :
                PlayFabResultCommon(),
                CharacterId(src.CharacterId),
                Inventory(src.Inventory),
                VirtualCurrency(src.VirtualCurrency),
                VirtualCurrencyRechargeTimes(src.VirtualCurrencyRechargeTimes)
            {}

            ~GetCharacterInventoryResult() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["CharacterId"], CharacterId);
                FromJsonUtilO(input["Inventory"], Inventory);
                FromJsonUtilP(input["VirtualCurrency"], VirtualCurrency);
                FromJsonUtilO(input["VirtualCurrencyRechargeTimes"], VirtualCurrencyRechargeTimes);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_CharacterId; ToJsonUtilS(CharacterId, each_CharacterId); output["CharacterId"] = each_CharacterId;
                Json::Value each_Inventory; ToJsonUtilO(Inventory, each_Inventory); output["Inventory"] = each_Inventory;
                Json::Value each_VirtualCurrency; ToJsonUtilP(VirtualCurrency, each_VirtualCurrency); output["VirtualCurrency"] = each_VirtualCurrency;
                Json::Value each_VirtualCurrencyRechargeTimes; ToJsonUtilO(VirtualCurrencyRechargeTimes, each_VirtualCurrencyRechargeTimes); output["VirtualCurrencyRechargeTimes"] = each_VirtualCurrencyRechargeTimes;
                return output;
            }
        };

        struct GetCharacterLeaderboardRequest : public PlayFabRequestCommon
        {
            std::string CharacterType;
            Boxed<Int32> MaxResultsCount;
            Int32 StartPosition;
            std::string StatisticName;

            GetCharacterLeaderboardRequest() :
                PlayFabRequestCommon(),
                CharacterType(),
                MaxResultsCount(),
                StartPosition(),
                StatisticName()
            {}

            GetCharacterLeaderboardRequest(const GetCharacterLeaderboardRequest& src) :
                PlayFabRequestCommon(),
                CharacterType(src.CharacterType),
                MaxResultsCount(src.MaxResultsCount),
                StartPosition(src.StartPosition),
                StatisticName(src.StatisticName)
            {}

            ~GetCharacterLeaderboardRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["CharacterType"], CharacterType);
                FromJsonUtilP(input["MaxResultsCount"], MaxResultsCount);
                FromJsonUtilP(input["StartPosition"], StartPosition);
                FromJsonUtilS(input["StatisticName"], StatisticName);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_CharacterType; ToJsonUtilS(CharacterType, each_CharacterType); output["CharacterType"] = each_CharacterType;
                Json::Value each_MaxResultsCount; ToJsonUtilP(MaxResultsCount, each_MaxResultsCount); output["MaxResultsCount"] = each_MaxResultsCount;
                Json::Value each_StartPosition; ToJsonUtilP(StartPosition, each_StartPosition); output["StartPosition"] = each_StartPosition;
                Json::Value each_StatisticName; ToJsonUtilS(StatisticName, each_StatisticName); output["StatisticName"] = each_StatisticName;
                return output;
            }
        };

        struct GetCharacterLeaderboardResult : public PlayFabResultCommon
        {
            std::list<CharacterLeaderboardEntry> Leaderboard;

            GetCharacterLeaderboardResult() :
                PlayFabResultCommon(),
                Leaderboard()
            {}

            GetCharacterLeaderboardResult(const GetCharacterLeaderboardResult& src) :
                PlayFabResultCommon(),
                Leaderboard(src.Leaderboard)
            {}

            ~GetCharacterLeaderboardResult() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilO(input["Leaderboard"], Leaderboard);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_Leaderboard; ToJsonUtilO(Leaderboard, each_Leaderboard); output["Leaderboard"] = each_Leaderboard;
                return output;
            }
        };

        struct GetCharacterStatisticsRequest : public PlayFabRequestCommon
        {
            std::string CharacterId;

            GetCharacterStatisticsRequest() :
                PlayFabRequestCommon(),
                CharacterId()
            {}

            GetCharacterStatisticsRequest(const GetCharacterStatisticsRequest& src) :
                PlayFabRequestCommon(),
                CharacterId(src.CharacterId)
            {}

            ~GetCharacterStatisticsRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["CharacterId"], CharacterId);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_CharacterId; ToJsonUtilS(CharacterId, each_CharacterId); output["CharacterId"] = each_CharacterId;
                return output;
            }
        };

        struct GetCharacterStatisticsResult : public PlayFabResultCommon
        {
            std::map<std::string, Int32> CharacterStatistics;

            GetCharacterStatisticsResult() :
                PlayFabResultCommon(),
                CharacterStatistics()
            {}

            GetCharacterStatisticsResult(const GetCharacterStatisticsResult& src) :
                PlayFabResultCommon(),
                CharacterStatistics(src.CharacterStatistics)
            {}

            ~GetCharacterStatisticsResult() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilP(input["CharacterStatistics"], CharacterStatistics);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_CharacterStatistics; ToJsonUtilP(CharacterStatistics, each_CharacterStatistics); output["CharacterStatistics"] = each_CharacterStatistics;
                return output;
            }
        };

        struct GetContentDownloadUrlRequest : public PlayFabRequestCommon
        {
            std::string HttpMethod;
            std::string Key;
            Boxed<bool> ThruCDN;

            GetContentDownloadUrlRequest() :
                PlayFabRequestCommon(),
                HttpMethod(),
                Key(),
                ThruCDN()
            {}

            GetContentDownloadUrlRequest(const GetContentDownloadUrlRequest& src) :
                PlayFabRequestCommon(),
                HttpMethod(src.HttpMethod),
                Key(src.Key),
                ThruCDN(src.ThruCDN)
            {}

            ~GetContentDownloadUrlRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["HttpMethod"], HttpMethod);
                FromJsonUtilS(input["Key"], Key);
                FromJsonUtilP(input["ThruCDN"], ThruCDN);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_HttpMethod; ToJsonUtilS(HttpMethod, each_HttpMethod); output["HttpMethod"] = each_HttpMethod;
                Json::Value each_Key; ToJsonUtilS(Key, each_Key); output["Key"] = each_Key;
                Json::Value each_ThruCDN; ToJsonUtilP(ThruCDN, each_ThruCDN); output["ThruCDN"] = each_ThruCDN;
                return output;
            }
        };

        struct GetContentDownloadUrlResult : public PlayFabResultCommon
        {
            std::string URL;

            GetContentDownloadUrlResult() :
                PlayFabResultCommon(),
                URL()
            {}

            GetContentDownloadUrlResult(const GetContentDownloadUrlResult& src) :
                PlayFabResultCommon(),
                URL(src.URL)
            {}

            ~GetContentDownloadUrlResult() = default;

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

        struct GetFriendLeaderboardAroundPlayerRequest : public PlayFabRequestCommon
        {
            std::map<std::string, std::string> CustomTags;
            Boxed<bool> IncludeFacebookFriends;
            Boxed<bool> IncludeSteamFriends;
            Boxed<Int32> MaxResultsCount;
            std::string PlayFabId;
            Boxed<PlayerProfileViewConstraints> ProfileConstraints;
            std::string StatisticName;
            Boxed<Int32> Version;
            std::string XboxToken;

            GetFriendLeaderboardAroundPlayerRequest() :
                PlayFabRequestCommon(),
                CustomTags(),
                IncludeFacebookFriends(),
                IncludeSteamFriends(),
                MaxResultsCount(),
                PlayFabId(),
                ProfileConstraints(),
                StatisticName(),
                Version(),
                XboxToken()
            {}

            GetFriendLeaderboardAroundPlayerRequest(const GetFriendLeaderboardAroundPlayerRequest& src) :
                PlayFabRequestCommon(),
                CustomTags(src.CustomTags),
                IncludeFacebookFriends(src.IncludeFacebookFriends),
                IncludeSteamFriends(src.IncludeSteamFriends),
                MaxResultsCount(src.MaxResultsCount),
                PlayFabId(src.PlayFabId),
                ProfileConstraints(src.ProfileConstraints),
                StatisticName(src.StatisticName),
                Version(src.Version),
                XboxToken(src.XboxToken)
            {}

            ~GetFriendLeaderboardAroundPlayerRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["CustomTags"], CustomTags);
                FromJsonUtilP(input["IncludeFacebookFriends"], IncludeFacebookFriends);
                FromJsonUtilP(input["IncludeSteamFriends"], IncludeSteamFriends);
                FromJsonUtilP(input["MaxResultsCount"], MaxResultsCount);
                FromJsonUtilS(input["PlayFabId"], PlayFabId);
                FromJsonUtilO(input["ProfileConstraints"], ProfileConstraints);
                FromJsonUtilS(input["StatisticName"], StatisticName);
                FromJsonUtilP(input["Version"], Version);
                FromJsonUtilS(input["XboxToken"], XboxToken);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_CustomTags; ToJsonUtilS(CustomTags, each_CustomTags); output["CustomTags"] = each_CustomTags;
                Json::Value each_IncludeFacebookFriends; ToJsonUtilP(IncludeFacebookFriends, each_IncludeFacebookFriends); output["IncludeFacebookFriends"] = each_IncludeFacebookFriends;
                Json::Value each_IncludeSteamFriends; ToJsonUtilP(IncludeSteamFriends, each_IncludeSteamFriends); output["IncludeSteamFriends"] = each_IncludeSteamFriends;
                Json::Value each_MaxResultsCount; ToJsonUtilP(MaxResultsCount, each_MaxResultsCount); output["MaxResultsCount"] = each_MaxResultsCount;
                Json::Value each_PlayFabId; ToJsonUtilS(PlayFabId, each_PlayFabId); output["PlayFabId"] = each_PlayFabId;
                Json::Value each_ProfileConstraints; ToJsonUtilO(ProfileConstraints, each_ProfileConstraints); output["ProfileConstraints"] = each_ProfileConstraints;
                Json::Value each_StatisticName; ToJsonUtilS(StatisticName, each_StatisticName); output["StatisticName"] = each_StatisticName;
                Json::Value each_Version; ToJsonUtilP(Version, each_Version); output["Version"] = each_Version;
                Json::Value each_XboxToken; ToJsonUtilS(XboxToken, each_XboxToken); output["XboxToken"] = each_XboxToken;
                return output;
            }
        };

        struct PlayerLeaderboardEntry : public PlayFabBaseModel
        {
            std::string DisplayName;
            std::string PlayFabId;
            Int32 Position;
            Boxed<PlayerProfileModel> Profile;
            Int32 StatValue;

            PlayerLeaderboardEntry() :
                PlayFabBaseModel(),
                DisplayName(),
                PlayFabId(),
                Position(),
                Profile(),
                StatValue()
            {}

            PlayerLeaderboardEntry(const PlayerLeaderboardEntry& src) :
                PlayFabBaseModel(),
                DisplayName(src.DisplayName),
                PlayFabId(src.PlayFabId),
                Position(src.Position),
                Profile(src.Profile),
                StatValue(src.StatValue)
            {}

            ~PlayerLeaderboardEntry() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["DisplayName"], DisplayName);
                FromJsonUtilS(input["PlayFabId"], PlayFabId);
                FromJsonUtilP(input["Position"], Position);
                FromJsonUtilO(input["Profile"], Profile);
                FromJsonUtilP(input["StatValue"], StatValue);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_DisplayName; ToJsonUtilS(DisplayName, each_DisplayName); output["DisplayName"] = each_DisplayName;
                Json::Value each_PlayFabId; ToJsonUtilS(PlayFabId, each_PlayFabId); output["PlayFabId"] = each_PlayFabId;
                Json::Value each_Position; ToJsonUtilP(Position, each_Position); output["Position"] = each_Position;
                Json::Value each_Profile; ToJsonUtilO(Profile, each_Profile); output["Profile"] = each_Profile;
                Json::Value each_StatValue; ToJsonUtilP(StatValue, each_StatValue); output["StatValue"] = each_StatValue;
                return output;
            }
        };

        struct GetFriendLeaderboardAroundPlayerResult : public PlayFabResultCommon
        {
            std::list<PlayerLeaderboardEntry> Leaderboard;
            Boxed<time_t> NextReset;
            Int32 Version;

            GetFriendLeaderboardAroundPlayerResult() :
                PlayFabResultCommon(),
                Leaderboard(),
                NextReset(),
                Version()
            {}

            GetFriendLeaderboardAroundPlayerResult(const GetFriendLeaderboardAroundPlayerResult& src) :
                PlayFabResultCommon(),
                Leaderboard(src.Leaderboard),
                NextReset(src.NextReset),
                Version(src.Version)
            {}

            ~GetFriendLeaderboardAroundPlayerResult() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilO(input["Leaderboard"], Leaderboard);
                FromJsonUtilT(input["NextReset"], NextReset);
                FromJsonUtilP(input["Version"], Version);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_Leaderboard; ToJsonUtilO(Leaderboard, each_Leaderboard); output["Leaderboard"] = each_Leaderboard;
                Json::Value each_NextReset; ToJsonUtilT(NextReset, each_NextReset); output["NextReset"] = each_NextReset;
                Json::Value each_Version; ToJsonUtilP(Version, each_Version); output["Version"] = each_Version;
                return output;
            }
        };

        struct GetFriendLeaderboardRequest : public PlayFabRequestCommon
        {
            std::map<std::string, std::string> CustomTags;
            Boxed<bool> IncludeFacebookFriends;
            Boxed<bool> IncludeSteamFriends;
            Boxed<Int32> MaxResultsCount;
            Boxed<PlayerProfileViewConstraints> ProfileConstraints;
            Int32 StartPosition;
            std::string StatisticName;
            Boxed<Int32> Version;
            std::string XboxToken;

            GetFriendLeaderboardRequest() :
                PlayFabRequestCommon(),
                CustomTags(),
                IncludeFacebookFriends(),
                IncludeSteamFriends(),
                MaxResultsCount(),
                ProfileConstraints(),
                StartPosition(),
                StatisticName(),
                Version(),
                XboxToken()
            {}

            GetFriendLeaderboardRequest(const GetFriendLeaderboardRequest& src) :
                PlayFabRequestCommon(),
                CustomTags(src.CustomTags),
                IncludeFacebookFriends(src.IncludeFacebookFriends),
                IncludeSteamFriends(src.IncludeSteamFriends),
                MaxResultsCount(src.MaxResultsCount),
                ProfileConstraints(src.ProfileConstraints),
                StartPosition(src.StartPosition),
                StatisticName(src.StatisticName),
                Version(src.Version),
                XboxToken(src.XboxToken)
            {}

            ~GetFriendLeaderboardRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["CustomTags"], CustomTags);
                FromJsonUtilP(input["IncludeFacebookFriends"], IncludeFacebookFriends);
                FromJsonUtilP(input["IncludeSteamFriends"], IncludeSteamFriends);
                FromJsonUtilP(input["MaxResultsCount"], MaxResultsCount);
                FromJsonUtilO(input["ProfileConstraints"], ProfileConstraints);
                FromJsonUtilP(input["StartPosition"], StartPosition);
                FromJsonUtilS(input["StatisticName"], StatisticName);
                FromJsonUtilP(input["Version"], Version);
                FromJsonUtilS(input["XboxToken"], XboxToken);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_CustomTags; ToJsonUtilS(CustomTags, each_CustomTags); output["CustomTags"] = each_CustomTags;
                Json::Value each_IncludeFacebookFriends; ToJsonUtilP(IncludeFacebookFriends, each_IncludeFacebookFriends); output["IncludeFacebookFriends"] = each_IncludeFacebookFriends;
                Json::Value each_IncludeSteamFriends; ToJsonUtilP(IncludeSteamFriends, each_IncludeSteamFriends); output["IncludeSteamFriends"] = each_IncludeSteamFriends;
                Json::Value each_MaxResultsCount; ToJsonUtilP(MaxResultsCount, each_MaxResultsCount); output["MaxResultsCount"] = each_MaxResultsCount;
                Json::Value each_ProfileConstraints; ToJsonUtilO(ProfileConstraints, each_ProfileConstraints); output["ProfileConstraints"] = each_ProfileConstraints;
                Json::Value each_StartPosition; ToJsonUtilP(StartPosition, each_StartPosition); output["StartPosition"] = each_StartPosition;
                Json::Value each_StatisticName; ToJsonUtilS(StatisticName, each_StatisticName); output["StatisticName"] = each_StatisticName;
                Json::Value each_Version; ToJsonUtilP(Version, each_Version); output["Version"] = each_Version;
                Json::Value each_XboxToken; ToJsonUtilS(XboxToken, each_XboxToken); output["XboxToken"] = each_XboxToken;
                return output;
            }
        };

        struct GetFriendsListRequest : public PlayFabRequestCommon
        {
            std::map<std::string, std::string> CustomTags;
            Boxed<bool> IncludeFacebookFriends;
            Boxed<bool> IncludeSteamFriends;
            Boxed<PlayerProfileViewConstraints> ProfileConstraints;
            std::string XboxToken;

            GetFriendsListRequest() :
                PlayFabRequestCommon(),
                CustomTags(),
                IncludeFacebookFriends(),
                IncludeSteamFriends(),
                ProfileConstraints(),
                XboxToken()
            {}

            GetFriendsListRequest(const GetFriendsListRequest& src) :
                PlayFabRequestCommon(),
                CustomTags(src.CustomTags),
                IncludeFacebookFriends(src.IncludeFacebookFriends),
                IncludeSteamFriends(src.IncludeSteamFriends),
                ProfileConstraints(src.ProfileConstraints),
                XboxToken(src.XboxToken)
            {}

            ~GetFriendsListRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["CustomTags"], CustomTags);
                FromJsonUtilP(input["IncludeFacebookFriends"], IncludeFacebookFriends);
                FromJsonUtilP(input["IncludeSteamFriends"], IncludeSteamFriends);
                FromJsonUtilO(input["ProfileConstraints"], ProfileConstraints);
                FromJsonUtilS(input["XboxToken"], XboxToken);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_CustomTags; ToJsonUtilS(CustomTags, each_CustomTags); output["CustomTags"] = each_CustomTags;
                Json::Value each_IncludeFacebookFriends; ToJsonUtilP(IncludeFacebookFriends, each_IncludeFacebookFriends); output["IncludeFacebookFriends"] = each_IncludeFacebookFriends;
                Json::Value each_IncludeSteamFriends; ToJsonUtilP(IncludeSteamFriends, each_IncludeSteamFriends); output["IncludeSteamFriends"] = each_IncludeSteamFriends;
                Json::Value each_ProfileConstraints; ToJsonUtilO(ProfileConstraints, each_ProfileConstraints); output["ProfileConstraints"] = each_ProfileConstraints;
                Json::Value each_XboxToken; ToJsonUtilS(XboxToken, each_XboxToken); output["XboxToken"] = each_XboxToken;
                return output;
            }
        };

        struct GetFriendsListResult : public PlayFabResultCommon
        {
            std::list<FriendInfo> Friends;

            GetFriendsListResult() :
                PlayFabResultCommon(),
                Friends()
            {}

            GetFriendsListResult(const GetFriendsListResult& src) :
                PlayFabResultCommon(),
                Friends(src.Friends)
            {}

            ~GetFriendsListResult() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilO(input["Friends"], Friends);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_Friends; ToJsonUtilO(Friends, each_Friends); output["Friends"] = each_Friends;
                return output;
            }
        };

        struct GetLeaderboardAroundCharacterRequest : public PlayFabRequestCommon
        {
            std::string CharacterId;
            std::string CharacterType;
            Boxed<Int32> MaxResultsCount;
            std::string StatisticName;

            GetLeaderboardAroundCharacterRequest() :
                PlayFabRequestCommon(),
                CharacterId(),
                CharacterType(),
                MaxResultsCount(),
                StatisticName()
            {}

            GetLeaderboardAroundCharacterRequest(const GetLeaderboardAroundCharacterRequest& src) :
                PlayFabRequestCommon(),
                CharacterId(src.CharacterId),
                CharacterType(src.CharacterType),
                MaxResultsCount(src.MaxResultsCount),
                StatisticName(src.StatisticName)
            {}

            ~GetLeaderboardAroundCharacterRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["CharacterId"], CharacterId);
                FromJsonUtilS(input["CharacterType"], CharacterType);
                FromJsonUtilP(input["MaxResultsCount"], MaxResultsCount);
                FromJsonUtilS(input["StatisticName"], StatisticName);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_CharacterId; ToJsonUtilS(CharacterId, each_CharacterId); output["CharacterId"] = each_CharacterId;
                Json::Value each_CharacterType; ToJsonUtilS(CharacterType, each_CharacterType); output["CharacterType"] = each_CharacterType;
                Json::Value each_MaxResultsCount; ToJsonUtilP(MaxResultsCount, each_MaxResultsCount); output["MaxResultsCount"] = each_MaxResultsCount;
                Json::Value each_StatisticName; ToJsonUtilS(StatisticName, each_StatisticName); output["StatisticName"] = each_StatisticName;
                return output;
            }
        };

        struct GetLeaderboardAroundCharacterResult : public PlayFabResultCommon
        {
            std::list<CharacterLeaderboardEntry> Leaderboard;

            GetLeaderboardAroundCharacterResult() :
                PlayFabResultCommon(),
                Leaderboard()
            {}

            GetLeaderboardAroundCharacterResult(const GetLeaderboardAroundCharacterResult& src) :
                PlayFabResultCommon(),
                Leaderboard(src.Leaderboard)
            {}

            ~GetLeaderboardAroundCharacterResult() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilO(input["Leaderboard"], Leaderboard);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_Leaderboard; ToJsonUtilO(Leaderboard, each_Leaderboard); output["Leaderboard"] = each_Leaderboard;
                return output;
            }
        };

        struct GetLeaderboardAroundPlayerRequest : public PlayFabRequestCommon
        {
            std::map<std::string, std::string> CustomTags;
            Boxed<Int32> MaxResultsCount;
            std::string PlayFabId;
            Boxed<PlayerProfileViewConstraints> ProfileConstraints;
            std::string StatisticName;
            Boxed<Int32> Version;

            GetLeaderboardAroundPlayerRequest() :
                PlayFabRequestCommon(),
                CustomTags(),
                MaxResultsCount(),
                PlayFabId(),
                ProfileConstraints(),
                StatisticName(),
                Version()
            {}

            GetLeaderboardAroundPlayerRequest(const GetLeaderboardAroundPlayerRequest& src) :
                PlayFabRequestCommon(),
                CustomTags(src.CustomTags),
                MaxResultsCount(src.MaxResultsCount),
                PlayFabId(src.PlayFabId),
                ProfileConstraints(src.ProfileConstraints),
                StatisticName(src.StatisticName),
                Version(src.Version)
            {}

            ~GetLeaderboardAroundPlayerRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["CustomTags"], CustomTags);
                FromJsonUtilP(input["MaxResultsCount"], MaxResultsCount);
                FromJsonUtilS(input["PlayFabId"], PlayFabId);
                FromJsonUtilO(input["ProfileConstraints"], ProfileConstraints);
                FromJsonUtilS(input["StatisticName"], StatisticName);
                FromJsonUtilP(input["Version"], Version);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_CustomTags; ToJsonUtilS(CustomTags, each_CustomTags); output["CustomTags"] = each_CustomTags;
                Json::Value each_MaxResultsCount; ToJsonUtilP(MaxResultsCount, each_MaxResultsCount); output["MaxResultsCount"] = each_MaxResultsCount;
                Json::Value each_PlayFabId; ToJsonUtilS(PlayFabId, each_PlayFabId); output["PlayFabId"] = each_PlayFabId;
                Json::Value each_ProfileConstraints; ToJsonUtilO(ProfileConstraints, each_ProfileConstraints); output["ProfileConstraints"] = each_ProfileConstraints;
                Json::Value each_StatisticName; ToJsonUtilS(StatisticName, each_StatisticName); output["StatisticName"] = each_StatisticName;
                Json::Value each_Version; ToJsonUtilP(Version, each_Version); output["Version"] = each_Version;
                return output;
            }
        };

        struct GetLeaderboardAroundPlayerResult : public PlayFabResultCommon
        {
            std::list<PlayerLeaderboardEntry> Leaderboard;
            Boxed<time_t> NextReset;
            Int32 Version;

            GetLeaderboardAroundPlayerResult() :
                PlayFabResultCommon(),
                Leaderboard(),
                NextReset(),
                Version()
            {}

            GetLeaderboardAroundPlayerResult(const GetLeaderboardAroundPlayerResult& src) :
                PlayFabResultCommon(),
                Leaderboard(src.Leaderboard),
                NextReset(src.NextReset),
                Version(src.Version)
            {}

            ~GetLeaderboardAroundPlayerResult() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilO(input["Leaderboard"], Leaderboard);
                FromJsonUtilT(input["NextReset"], NextReset);
                FromJsonUtilP(input["Version"], Version);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_Leaderboard; ToJsonUtilO(Leaderboard, each_Leaderboard); output["Leaderboard"] = each_Leaderboard;
                Json::Value each_NextReset; ToJsonUtilT(NextReset, each_NextReset); output["NextReset"] = each_NextReset;
                Json::Value each_Version; ToJsonUtilP(Version, each_Version); output["Version"] = each_Version;
                return output;
            }
        };

        struct GetLeaderboardForUsersCharactersRequest : public PlayFabRequestCommon
        {
            std::string StatisticName;

            GetLeaderboardForUsersCharactersRequest() :
                PlayFabRequestCommon(),
                StatisticName()
            {}

            GetLeaderboardForUsersCharactersRequest(const GetLeaderboardForUsersCharactersRequest& src) :
                PlayFabRequestCommon(),
                StatisticName(src.StatisticName)
            {}

            ~GetLeaderboardForUsersCharactersRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["StatisticName"], StatisticName);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_StatisticName; ToJsonUtilS(StatisticName, each_StatisticName); output["StatisticName"] = each_StatisticName;
                return output;
            }
        };

        struct GetLeaderboardForUsersCharactersResult : public PlayFabResultCommon
        {
            std::list<CharacterLeaderboardEntry> Leaderboard;

            GetLeaderboardForUsersCharactersResult() :
                PlayFabResultCommon(),
                Leaderboard()
            {}

            GetLeaderboardForUsersCharactersResult(const GetLeaderboardForUsersCharactersResult& src) :
                PlayFabResultCommon(),
                Leaderboard(src.Leaderboard)
            {}

            ~GetLeaderboardForUsersCharactersResult() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilO(input["Leaderboard"], Leaderboard);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_Leaderboard; ToJsonUtilO(Leaderboard, each_Leaderboard); output["Leaderboard"] = each_Leaderboard;
                return output;
            }
        };

        struct GetLeaderboardRequest : public PlayFabRequestCommon
        {
            std::map<std::string, std::string> CustomTags;
            Boxed<Int32> MaxResultsCount;
            Boxed<PlayerProfileViewConstraints> ProfileConstraints;
            Int32 StartPosition;
            std::string StatisticName;
            Boxed<Int32> Version;

            GetLeaderboardRequest() :
                PlayFabRequestCommon(),
                CustomTags(),
                MaxResultsCount(),
                ProfileConstraints(),
                StartPosition(),
                StatisticName(),
                Version()
            {}

            GetLeaderboardRequest(const GetLeaderboardRequest& src) :
                PlayFabRequestCommon(),
                CustomTags(src.CustomTags),
                MaxResultsCount(src.MaxResultsCount),
                ProfileConstraints(src.ProfileConstraints),
                StartPosition(src.StartPosition),
                StatisticName(src.StatisticName),
                Version(src.Version)
            {}

            ~GetLeaderboardRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["CustomTags"], CustomTags);
                FromJsonUtilP(input["MaxResultsCount"], MaxResultsCount);
                FromJsonUtilO(input["ProfileConstraints"], ProfileConstraints);
                FromJsonUtilP(input["StartPosition"], StartPosition);
                FromJsonUtilS(input["StatisticName"], StatisticName);
                FromJsonUtilP(input["Version"], Version);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_CustomTags; ToJsonUtilS(CustomTags, each_CustomTags); output["CustomTags"] = each_CustomTags;
                Json::Value each_MaxResultsCount; ToJsonUtilP(MaxResultsCount, each_MaxResultsCount); output["MaxResultsCount"] = each_MaxResultsCount;
                Json::Value each_ProfileConstraints; ToJsonUtilO(ProfileConstraints, each_ProfileConstraints); output["ProfileConstraints"] = each_ProfileConstraints;
                Json::Value each_StartPosition; ToJsonUtilP(StartPosition, each_StartPosition); output["StartPosition"] = each_StartPosition;
                Json::Value each_StatisticName; ToJsonUtilS(StatisticName, each_StatisticName); output["StatisticName"] = each_StatisticName;
                Json::Value each_Version; ToJsonUtilP(Version, each_Version); output["Version"] = each_Version;
                return output;
            }
        };

        struct GetLeaderboardResult : public PlayFabResultCommon
        {
            std::list<PlayerLeaderboardEntry> Leaderboard;
            Boxed<time_t> NextReset;
            Int32 Version;

            GetLeaderboardResult() :
                PlayFabResultCommon(),
                Leaderboard(),
                NextReset(),
                Version()
            {}

            GetLeaderboardResult(const GetLeaderboardResult& src) :
                PlayFabResultCommon(),
                Leaderboard(src.Leaderboard),
                NextReset(src.NextReset),
                Version(src.Version)
            {}

            ~GetLeaderboardResult() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilO(input["Leaderboard"], Leaderboard);
                FromJsonUtilT(input["NextReset"], NextReset);
                FromJsonUtilP(input["Version"], Version);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_Leaderboard; ToJsonUtilO(Leaderboard, each_Leaderboard); output["Leaderboard"] = each_Leaderboard;
                Json::Value each_NextReset; ToJsonUtilT(NextReset, each_NextReset); output["NextReset"] = each_NextReset;
                Json::Value each_Version; ToJsonUtilP(Version, each_Version); output["Version"] = each_Version;
                return output;
            }
        };

        struct GetPaymentTokenRequest : public PlayFabRequestCommon
        {
            std::string TokenProvider;

            GetPaymentTokenRequest() :
                PlayFabRequestCommon(),
                TokenProvider()
            {}

            GetPaymentTokenRequest(const GetPaymentTokenRequest& src) :
                PlayFabRequestCommon(),
                TokenProvider(src.TokenProvider)
            {}

            ~GetPaymentTokenRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["TokenProvider"], TokenProvider);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_TokenProvider; ToJsonUtilS(TokenProvider, each_TokenProvider); output["TokenProvider"] = each_TokenProvider;
                return output;
            }
        };

        struct GetPaymentTokenResult : public PlayFabResultCommon
        {
            std::string OrderId;
            std::string ProviderToken;

            GetPaymentTokenResult() :
                PlayFabResultCommon(),
                OrderId(),
                ProviderToken()
            {}

            GetPaymentTokenResult(const GetPaymentTokenResult& src) :
                PlayFabResultCommon(),
                OrderId(src.OrderId),
                ProviderToken(src.ProviderToken)
            {}

            ~GetPaymentTokenResult() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["OrderId"], OrderId);
                FromJsonUtilS(input["ProviderToken"], ProviderToken);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_OrderId; ToJsonUtilS(OrderId, each_OrderId); output["OrderId"] = each_OrderId;
                Json::Value each_ProviderToken; ToJsonUtilS(ProviderToken, each_ProviderToken); output["ProviderToken"] = each_ProviderToken;
                return output;
            }
        };

        struct GetPhotonAuthenticationTokenRequest : public PlayFabRequestCommon
        {
            std::string PhotonApplicationId;

            GetPhotonAuthenticationTokenRequest() :
                PlayFabRequestCommon(),
                PhotonApplicationId()
            {}

            GetPhotonAuthenticationTokenRequest(const GetPhotonAuthenticationTokenRequest& src) :
                PlayFabRequestCommon(),
                PhotonApplicationId(src.PhotonApplicationId)
            {}

            ~GetPhotonAuthenticationTokenRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["PhotonApplicationId"], PhotonApplicationId);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_PhotonApplicationId; ToJsonUtilS(PhotonApplicationId, each_PhotonApplicationId); output["PhotonApplicationId"] = each_PhotonApplicationId;
                return output;
            }
        };

        struct GetPhotonAuthenticationTokenResult : public PlayFabResultCommon
        {
            std::string PhotonCustomAuthenticationToken;

            GetPhotonAuthenticationTokenResult() :
                PlayFabResultCommon(),
                PhotonCustomAuthenticationToken()
            {}

            GetPhotonAuthenticationTokenResult(const GetPhotonAuthenticationTokenResult& src) :
                PlayFabResultCommon(),
                PhotonCustomAuthenticationToken(src.PhotonCustomAuthenticationToken)
            {}

            ~GetPhotonAuthenticationTokenResult() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["PhotonCustomAuthenticationToken"], PhotonCustomAuthenticationToken);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_PhotonCustomAuthenticationToken; ToJsonUtilS(PhotonCustomAuthenticationToken, each_PhotonCustomAuthenticationToken); output["PhotonCustomAuthenticationToken"] = each_PhotonCustomAuthenticationToken;
                return output;
            }
        };

        struct GetPlayerCombinedInfoRequestParams : public PlayFabBaseModel
        {
            bool GetCharacterInventories;
            bool GetCharacterList;
            bool GetPlayerProfile;
            bool GetPlayerStatistics;
            bool GetTitleData;
            bool GetUserAccountInfo;
            bool GetUserData;
            bool GetUserInventory;
            bool GetUserReadOnlyData;
            bool GetUserVirtualCurrency;
            std::list<std::string> PlayerStatisticNames;
            Boxed<PlayerProfileViewConstraints> ProfileConstraints;
            std::list<std::string> TitleDataKeys;
            std::list<std::string> UserDataKeys;
            std::list<std::string> UserReadOnlyDataKeys;

            GetPlayerCombinedInfoRequestParams() :
                PlayFabBaseModel(),
                GetCharacterInventories(),
                GetCharacterList(),
                GetPlayerProfile(),
                GetPlayerStatistics(),
                GetTitleData(),
                GetUserAccountInfo(),
                GetUserData(),
                GetUserInventory(),
                GetUserReadOnlyData(),
                GetUserVirtualCurrency(),
                PlayerStatisticNames(),
                ProfileConstraints(),
                TitleDataKeys(),
                UserDataKeys(),
                UserReadOnlyDataKeys()
            {}

            GetPlayerCombinedInfoRequestParams(const GetPlayerCombinedInfoRequestParams& src) :
                PlayFabBaseModel(),
                GetCharacterInventories(src.GetCharacterInventories),
                GetCharacterList(src.GetCharacterList),
                GetPlayerProfile(src.GetPlayerProfile),
                GetPlayerStatistics(src.GetPlayerStatistics),
                GetTitleData(src.GetTitleData),
                GetUserAccountInfo(src.GetUserAccountInfo),
                GetUserData(src.GetUserData),
                GetUserInventory(src.GetUserInventory),
                GetUserReadOnlyData(src.GetUserReadOnlyData),
                GetUserVirtualCurrency(src.GetUserVirtualCurrency),
                PlayerStatisticNames(src.PlayerStatisticNames),
                ProfileConstraints(src.ProfileConstraints),
                TitleDataKeys(src.TitleDataKeys),
                UserDataKeys(src.UserDataKeys),
                UserReadOnlyDataKeys(src.UserReadOnlyDataKeys)
            {}

            ~GetPlayerCombinedInfoRequestParams() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilP(input["GetCharacterInventories"], GetCharacterInventories);
                FromJsonUtilP(input["GetCharacterList"], GetCharacterList);
                FromJsonUtilP(input["GetPlayerProfile"], GetPlayerProfile);
                FromJsonUtilP(input["GetPlayerStatistics"], GetPlayerStatistics);
                FromJsonUtilP(input["GetTitleData"], GetTitleData);
                FromJsonUtilP(input["GetUserAccountInfo"], GetUserAccountInfo);
                FromJsonUtilP(input["GetUserData"], GetUserData);
                FromJsonUtilP(input["GetUserInventory"], GetUserInventory);
                FromJsonUtilP(input["GetUserReadOnlyData"], GetUserReadOnlyData);
                FromJsonUtilP(input["GetUserVirtualCurrency"], GetUserVirtualCurrency);
                FromJsonUtilS(input["PlayerStatisticNames"], PlayerStatisticNames);
                FromJsonUtilO(input["ProfileConstraints"], ProfileConstraints);
                FromJsonUtilS(input["TitleDataKeys"], TitleDataKeys);
                FromJsonUtilS(input["UserDataKeys"], UserDataKeys);
                FromJsonUtilS(input["UserReadOnlyDataKeys"], UserReadOnlyDataKeys);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_GetCharacterInventories; ToJsonUtilP(GetCharacterInventories, each_GetCharacterInventories); output["GetCharacterInventories"] = each_GetCharacterInventories;
                Json::Value each_GetCharacterList; ToJsonUtilP(GetCharacterList, each_GetCharacterList); output["GetCharacterList"] = each_GetCharacterList;
                Json::Value each_GetPlayerProfile; ToJsonUtilP(GetPlayerProfile, each_GetPlayerProfile); output["GetPlayerProfile"] = each_GetPlayerProfile;
                Json::Value each_GetPlayerStatistics; ToJsonUtilP(GetPlayerStatistics, each_GetPlayerStatistics); output["GetPlayerStatistics"] = each_GetPlayerStatistics;
                Json::Value each_GetTitleData; ToJsonUtilP(GetTitleData, each_GetTitleData); output["GetTitleData"] = each_GetTitleData;
                Json::Value each_GetUserAccountInfo; ToJsonUtilP(GetUserAccountInfo, each_GetUserAccountInfo); output["GetUserAccountInfo"] = each_GetUserAccountInfo;
                Json::Value each_GetUserData; ToJsonUtilP(GetUserData, each_GetUserData); output["GetUserData"] = each_GetUserData;
                Json::Value each_GetUserInventory; ToJsonUtilP(GetUserInventory, each_GetUserInventory); output["GetUserInventory"] = each_GetUserInventory;
                Json::Value each_GetUserReadOnlyData; ToJsonUtilP(GetUserReadOnlyData, each_GetUserReadOnlyData); output["GetUserReadOnlyData"] = each_GetUserReadOnlyData;
                Json::Value each_GetUserVirtualCurrency; ToJsonUtilP(GetUserVirtualCurrency, each_GetUserVirtualCurrency); output["GetUserVirtualCurrency"] = each_GetUserVirtualCurrency;
                Json::Value each_PlayerStatisticNames; ToJsonUtilS(PlayerStatisticNames, each_PlayerStatisticNames); output["PlayerStatisticNames"] = each_PlayerStatisticNames;
                Json::Value each_ProfileConstraints; ToJsonUtilO(ProfileConstraints, each_ProfileConstraints); output["ProfileConstraints"] = each_ProfileConstraints;
                Json::Value each_TitleDataKeys; ToJsonUtilS(TitleDataKeys, each_TitleDataKeys); output["TitleDataKeys"] = each_TitleDataKeys;
                Json::Value each_UserDataKeys; ToJsonUtilS(UserDataKeys, each_UserDataKeys); output["UserDataKeys"] = each_UserDataKeys;
                Json::Value each_UserReadOnlyDataKeys; ToJsonUtilS(UserReadOnlyDataKeys, each_UserReadOnlyDataKeys); output["UserReadOnlyDataKeys"] = each_UserReadOnlyDataKeys;
                return output;
            }
        };

        struct GetPlayerCombinedInfoRequest : public PlayFabRequestCommon
        {
            std::map<std::string, std::string> CustomTags;
            GetPlayerCombinedInfoRequestParams InfoRequestParameters;
            std::string PlayFabId;

            GetPlayerCombinedInfoRequest() :
                PlayFabRequestCommon(),
                CustomTags(),
                InfoRequestParameters(),
                PlayFabId()
            {}

            GetPlayerCombinedInfoRequest(const GetPlayerCombinedInfoRequest& src) :
                PlayFabRequestCommon(),
                CustomTags(src.CustomTags),
                InfoRequestParameters(src.InfoRequestParameters),
                PlayFabId(src.PlayFabId)
            {}

            ~GetPlayerCombinedInfoRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["CustomTags"], CustomTags);
                FromJsonUtilO(input["InfoRequestParameters"], InfoRequestParameters);
                FromJsonUtilS(input["PlayFabId"], PlayFabId);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_CustomTags; ToJsonUtilS(CustomTags, each_CustomTags); output["CustomTags"] = each_CustomTags;
                Json::Value each_InfoRequestParameters; ToJsonUtilO(InfoRequestParameters, each_InfoRequestParameters); output["InfoRequestParameters"] = each_InfoRequestParameters;
                Json::Value each_PlayFabId; ToJsonUtilS(PlayFabId, each_PlayFabId); output["PlayFabId"] = each_PlayFabId;
                return output;
            }
        };

        struct StatisticValue : public PlayFabBaseModel
        {
            std::string StatisticName;
            Int32 Value;
            Uint32 Version;

            StatisticValue() :
                PlayFabBaseModel(),
                StatisticName(),
                Value(),
                Version()
            {}

            StatisticValue(const StatisticValue& src) :
                PlayFabBaseModel(),
                StatisticName(src.StatisticName),
                Value(src.Value),
                Version(src.Version)
            {}

            ~StatisticValue() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["StatisticName"], StatisticName);
                FromJsonUtilP(input["Value"], Value);
                FromJsonUtilP(input["Version"], Version);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_StatisticName; ToJsonUtilS(StatisticName, each_StatisticName); output["StatisticName"] = each_StatisticName;
                Json::Value each_Value; ToJsonUtilP(Value, each_Value); output["Value"] = each_Value;
                Json::Value each_Version; ToJsonUtilP(Version, each_Version); output["Version"] = each_Version;
                return output;
            }
        };

        struct GetPlayerCombinedInfoResultPayload : public PlayFabBaseModel
        {
            Boxed<UserAccountInfo> AccountInfo;
            std::list<CharacterInventory> CharacterInventories;
            std::list<CharacterResult> CharacterList;
            Boxed<PlayerProfileModel> PlayerProfile;
            std::list<StatisticValue> PlayerStatistics;
            std::map<std::string, std::string> TitleData;
            std::map<std::string, UserDataRecord> UserData;
            Uint32 UserDataVersion;
            std::list<ItemInstance> UserInventory;
            std::map<std::string, UserDataRecord> UserReadOnlyData;
            Uint32 UserReadOnlyDataVersion;
            std::map<std::string, Int32> UserVirtualCurrency;
            std::map<std::string, VirtualCurrencyRechargeTime> UserVirtualCurrencyRechargeTimes;

            GetPlayerCombinedInfoResultPayload() :
                PlayFabBaseModel(),
                AccountInfo(),
                CharacterInventories(),
                CharacterList(),
                PlayerProfile(),
                PlayerStatistics(),
                TitleData(),
                UserData(),
                UserDataVersion(),
                UserInventory(),
                UserReadOnlyData(),
                UserReadOnlyDataVersion(),
                UserVirtualCurrency(),
                UserVirtualCurrencyRechargeTimes()
            {}

            GetPlayerCombinedInfoResultPayload(const GetPlayerCombinedInfoResultPayload& src) :
                PlayFabBaseModel(),
                AccountInfo(src.AccountInfo),
                CharacterInventories(src.CharacterInventories),
                CharacterList(src.CharacterList),
                PlayerProfile(src.PlayerProfile),
                PlayerStatistics(src.PlayerStatistics),
                TitleData(src.TitleData),
                UserData(src.UserData),
                UserDataVersion(src.UserDataVersion),
                UserInventory(src.UserInventory),
                UserReadOnlyData(src.UserReadOnlyData),
                UserReadOnlyDataVersion(src.UserReadOnlyDataVersion),
                UserVirtualCurrency(src.UserVirtualCurrency),
                UserVirtualCurrencyRechargeTimes(src.UserVirtualCurrencyRechargeTimes)
            {}

            ~GetPlayerCombinedInfoResultPayload() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilO(input["AccountInfo"], AccountInfo);
                FromJsonUtilO(input["CharacterInventories"], CharacterInventories);
                FromJsonUtilO(input["CharacterList"], CharacterList);
                FromJsonUtilO(input["PlayerProfile"], PlayerProfile);
                FromJsonUtilO(input["PlayerStatistics"], PlayerStatistics);
                FromJsonUtilS(input["TitleData"], TitleData);
                FromJsonUtilO(input["UserData"], UserData);
                FromJsonUtilP(input["UserDataVersion"], UserDataVersion);
                FromJsonUtilO(input["UserInventory"], UserInventory);
                FromJsonUtilO(input["UserReadOnlyData"], UserReadOnlyData);
                FromJsonUtilP(input["UserReadOnlyDataVersion"], UserReadOnlyDataVersion);
                FromJsonUtilP(input["UserVirtualCurrency"], UserVirtualCurrency);
                FromJsonUtilO(input["UserVirtualCurrencyRechargeTimes"], UserVirtualCurrencyRechargeTimes);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_AccountInfo; ToJsonUtilO(AccountInfo, each_AccountInfo); output["AccountInfo"] = each_AccountInfo;
                Json::Value each_CharacterInventories; ToJsonUtilO(CharacterInventories, each_CharacterInventories); output["CharacterInventories"] = each_CharacterInventories;
                Json::Value each_CharacterList; ToJsonUtilO(CharacterList, each_CharacterList); output["CharacterList"] = each_CharacterList;
                Json::Value each_PlayerProfile; ToJsonUtilO(PlayerProfile, each_PlayerProfile); output["PlayerProfile"] = each_PlayerProfile;
                Json::Value each_PlayerStatistics; ToJsonUtilO(PlayerStatistics, each_PlayerStatistics); output["PlayerStatistics"] = each_PlayerStatistics;
                Json::Value each_TitleData; ToJsonUtilS(TitleData, each_TitleData); output["TitleData"] = each_TitleData;
                Json::Value each_UserData; ToJsonUtilO(UserData, each_UserData); output["UserData"] = each_UserData;
                Json::Value each_UserDataVersion; ToJsonUtilP(UserDataVersion, each_UserDataVersion); output["UserDataVersion"] = each_UserDataVersion;
                Json::Value each_UserInventory; ToJsonUtilO(UserInventory, each_UserInventory); output["UserInventory"] = each_UserInventory;
                Json::Value each_UserReadOnlyData; ToJsonUtilO(UserReadOnlyData, each_UserReadOnlyData); output["UserReadOnlyData"] = each_UserReadOnlyData;
                Json::Value each_UserReadOnlyDataVersion; ToJsonUtilP(UserReadOnlyDataVersion, each_UserReadOnlyDataVersion); output["UserReadOnlyDataVersion"] = each_UserReadOnlyDataVersion;
                Json::Value each_UserVirtualCurrency; ToJsonUtilP(UserVirtualCurrency, each_UserVirtualCurrency); output["UserVirtualCurrency"] = each_UserVirtualCurrency;
                Json::Value each_UserVirtualCurrencyRechargeTimes; ToJsonUtilO(UserVirtualCurrencyRechargeTimes, each_UserVirtualCurrencyRechargeTimes); output["UserVirtualCurrencyRechargeTimes"] = each_UserVirtualCurrencyRechargeTimes;
                return output;
            }
        };

        struct GetPlayerCombinedInfoResult : public PlayFabResultCommon
        {
            Boxed<GetPlayerCombinedInfoResultPayload> InfoResultPayload;
            std::string PlayFabId;

            GetPlayerCombinedInfoResult() :
                PlayFabResultCommon(),
                InfoResultPayload(),
                PlayFabId()
            {}

            GetPlayerCombinedInfoResult(const GetPlayerCombinedInfoResult& src) :
                PlayFabResultCommon(),
                InfoResultPayload(src.InfoResultPayload),
                PlayFabId(src.PlayFabId)
            {}

            ~GetPlayerCombinedInfoResult() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilO(input["InfoResultPayload"], InfoResultPayload);
                FromJsonUtilS(input["PlayFabId"], PlayFabId);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_InfoResultPayload; ToJsonUtilO(InfoResultPayload, each_InfoResultPayload); output["InfoResultPayload"] = each_InfoResultPayload;
                Json::Value each_PlayFabId; ToJsonUtilS(PlayFabId, each_PlayFabId); output["PlayFabId"] = each_PlayFabId;
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

        struct GetPlayerSegmentsRequest : public PlayFabRequestCommon
        {

            GetPlayerSegmentsRequest() :
                PlayFabRequestCommon()
            {}

            GetPlayerSegmentsRequest(const GetPlayerSegmentsRequest&) :
                PlayFabRequestCommon()
            {}

            ~GetPlayerSegmentsRequest() = default;

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

        struct StatisticNameVersion : public PlayFabBaseModel
        {
            std::string StatisticName;
            Uint32 Version;

            StatisticNameVersion() :
                PlayFabBaseModel(),
                StatisticName(),
                Version()
            {}

            StatisticNameVersion(const StatisticNameVersion& src) :
                PlayFabBaseModel(),
                StatisticName(src.StatisticName),
                Version(src.Version)
            {}

            ~StatisticNameVersion() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["StatisticName"], StatisticName);
                FromJsonUtilP(input["Version"], Version);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_StatisticName; ToJsonUtilS(StatisticName, each_StatisticName); output["StatisticName"] = each_StatisticName;
                Json::Value each_Version; ToJsonUtilP(Version, each_Version); output["Version"] = each_Version;
                return output;
            }
        };

        struct GetPlayerStatisticsRequest : public PlayFabRequestCommon
        {
            std::map<std::string, std::string> CustomTags;
            std::list<std::string> StatisticNames;
            std::list<StatisticNameVersion> StatisticNameVersions;

            GetPlayerStatisticsRequest() :
                PlayFabRequestCommon(),
                CustomTags(),
                StatisticNames(),
                StatisticNameVersions()
            {}

            GetPlayerStatisticsRequest(const GetPlayerStatisticsRequest& src) :
                PlayFabRequestCommon(),
                CustomTags(src.CustomTags),
                StatisticNames(src.StatisticNames),
                StatisticNameVersions(src.StatisticNameVersions)
            {}

            ~GetPlayerStatisticsRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["CustomTags"], CustomTags);
                FromJsonUtilS(input["StatisticNames"], StatisticNames);
                FromJsonUtilO(input["StatisticNameVersions"], StatisticNameVersions);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_CustomTags; ToJsonUtilS(CustomTags, each_CustomTags); output["CustomTags"] = each_CustomTags;
                Json::Value each_StatisticNames; ToJsonUtilS(StatisticNames, each_StatisticNames); output["StatisticNames"] = each_StatisticNames;
                Json::Value each_StatisticNameVersions; ToJsonUtilO(StatisticNameVersions, each_StatisticNameVersions); output["StatisticNameVersions"] = each_StatisticNameVersions;
                return output;
            }
        };

        struct GetPlayerStatisticsResult : public PlayFabResultCommon
        {
            std::list<StatisticValue> Statistics;

            GetPlayerStatisticsResult() :
                PlayFabResultCommon(),
                Statistics()
            {}

            GetPlayerStatisticsResult(const GetPlayerStatisticsResult& src) :
                PlayFabResultCommon(),
                Statistics(src.Statistics)
            {}

            ~GetPlayerStatisticsResult() = default;

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
            Boxed<time_t> DeactivationTime;
            Boxed<time_t> ScheduledActivationTime;
            Boxed<time_t> ScheduledDeactivationTime;
            std::string StatisticName;
            Uint32 Version;

            PlayerStatisticVersion() :
                PlayFabBaseModel(),
                ActivationTime(),
                DeactivationTime(),
                ScheduledActivationTime(),
                ScheduledDeactivationTime(),
                StatisticName(),
                Version()
            {}

            PlayerStatisticVersion(const PlayerStatisticVersion& src) :
                PlayFabBaseModel(),
                ActivationTime(src.ActivationTime),
                DeactivationTime(src.DeactivationTime),
                ScheduledActivationTime(src.ScheduledActivationTime),
                ScheduledDeactivationTime(src.ScheduledDeactivationTime),
                StatisticName(src.StatisticName),
                Version(src.Version)
            {}

            ~PlayerStatisticVersion() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilT(input["ActivationTime"], ActivationTime);
                FromJsonUtilT(input["DeactivationTime"], DeactivationTime);
                FromJsonUtilT(input["ScheduledActivationTime"], ScheduledActivationTime);
                FromJsonUtilT(input["ScheduledDeactivationTime"], ScheduledDeactivationTime);
                FromJsonUtilS(input["StatisticName"], StatisticName);
                FromJsonUtilP(input["Version"], Version);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_ActivationTime; ToJsonUtilT(ActivationTime, each_ActivationTime); output["ActivationTime"] = each_ActivationTime;
                Json::Value each_DeactivationTime; ToJsonUtilT(DeactivationTime, each_DeactivationTime); output["DeactivationTime"] = each_DeactivationTime;
                Json::Value each_ScheduledActivationTime; ToJsonUtilT(ScheduledActivationTime, each_ScheduledActivationTime); output["ScheduledActivationTime"] = each_ScheduledActivationTime;
                Json::Value each_ScheduledDeactivationTime; ToJsonUtilT(ScheduledDeactivationTime, each_ScheduledDeactivationTime); output["ScheduledDeactivationTime"] = each_ScheduledDeactivationTime;
                Json::Value each_StatisticName; ToJsonUtilS(StatisticName, each_StatisticName); output["StatisticName"] = each_StatisticName;
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

        struct GetPlayerTradesRequest : public PlayFabRequestCommon
        {
            Boxed<TradeStatus> StatusFilter;

            GetPlayerTradesRequest() :
                PlayFabRequestCommon(),
                StatusFilter()
            {}

            GetPlayerTradesRequest(const GetPlayerTradesRequest& src) :
                PlayFabRequestCommon(),
                StatusFilter(src.StatusFilter)
            {}

            ~GetPlayerTradesRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilE(input["StatusFilter"], StatusFilter);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_StatusFilter; ToJsonUtilE(StatusFilter, each_StatusFilter); output["StatusFilter"] = each_StatusFilter;
                return output;
            }
        };

        struct GetPlayerTradesResponse : public PlayFabResultCommon
        {
            std::list<TradeInfo> AcceptedTrades;
            std::list<TradeInfo> OpenedTrades;

            GetPlayerTradesResponse() :
                PlayFabResultCommon(),
                AcceptedTrades(),
                OpenedTrades()
            {}

            GetPlayerTradesResponse(const GetPlayerTradesResponse& src) :
                PlayFabResultCommon(),
                AcceptedTrades(src.AcceptedTrades),
                OpenedTrades(src.OpenedTrades)
            {}

            ~GetPlayerTradesResponse() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilO(input["AcceptedTrades"], AcceptedTrades);
                FromJsonUtilO(input["OpenedTrades"], OpenedTrades);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_AcceptedTrades; ToJsonUtilO(AcceptedTrades, each_AcceptedTrades); output["AcceptedTrades"] = each_AcceptedTrades;
                Json::Value each_OpenedTrades; ToJsonUtilO(OpenedTrades, each_OpenedTrades); output["OpenedTrades"] = each_OpenedTrades;
                return output;
            }
        };

        struct GetPlayFabIDsFromFacebookIDsRequest : public PlayFabRequestCommon
        {
            std::list<std::string> FacebookIDs;

            GetPlayFabIDsFromFacebookIDsRequest() :
                PlayFabRequestCommon(),
                FacebookIDs()
            {}

            GetPlayFabIDsFromFacebookIDsRequest(const GetPlayFabIDsFromFacebookIDsRequest& src) :
                PlayFabRequestCommon(),
                FacebookIDs(src.FacebookIDs)
            {}

            ~GetPlayFabIDsFromFacebookIDsRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["FacebookIDs"], FacebookIDs);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_FacebookIDs; ToJsonUtilS(FacebookIDs, each_FacebookIDs); output["FacebookIDs"] = each_FacebookIDs;
                return output;
            }
        };

        struct GetPlayFabIDsFromFacebookIDsResult : public PlayFabResultCommon
        {
            std::list<FacebookPlayFabIdPair> Data;

            GetPlayFabIDsFromFacebookIDsResult() :
                PlayFabResultCommon(),
                Data()
            {}

            GetPlayFabIDsFromFacebookIDsResult(const GetPlayFabIDsFromFacebookIDsResult& src) :
                PlayFabResultCommon(),
                Data(src.Data)
            {}

            ~GetPlayFabIDsFromFacebookIDsResult() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilO(input["Data"], Data);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_Data; ToJsonUtilO(Data, each_Data); output["Data"] = each_Data;
                return output;
            }
        };

        struct GetPlayFabIDsFromFacebookInstantGamesIdsRequest : public PlayFabRequestCommon
        {
            std::list<std::string> FacebookInstantGamesIds;

            GetPlayFabIDsFromFacebookInstantGamesIdsRequest() :
                PlayFabRequestCommon(),
                FacebookInstantGamesIds()
            {}

            GetPlayFabIDsFromFacebookInstantGamesIdsRequest(const GetPlayFabIDsFromFacebookInstantGamesIdsRequest& src) :
                PlayFabRequestCommon(),
                FacebookInstantGamesIds(src.FacebookInstantGamesIds)
            {}

            ~GetPlayFabIDsFromFacebookInstantGamesIdsRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["FacebookInstantGamesIds"], FacebookInstantGamesIds);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_FacebookInstantGamesIds; ToJsonUtilS(FacebookInstantGamesIds, each_FacebookInstantGamesIds); output["FacebookInstantGamesIds"] = each_FacebookInstantGamesIds;
                return output;
            }
        };

        struct GetPlayFabIDsFromFacebookInstantGamesIdsResult : public PlayFabResultCommon
        {
            std::list<FacebookInstantGamesPlayFabIdPair> Data;

            GetPlayFabIDsFromFacebookInstantGamesIdsResult() :
                PlayFabResultCommon(),
                Data()
            {}

            GetPlayFabIDsFromFacebookInstantGamesIdsResult(const GetPlayFabIDsFromFacebookInstantGamesIdsResult& src) :
                PlayFabResultCommon(),
                Data(src.Data)
            {}

            ~GetPlayFabIDsFromFacebookInstantGamesIdsResult() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilO(input["Data"], Data);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_Data; ToJsonUtilO(Data, each_Data); output["Data"] = each_Data;
                return output;
            }
        };

        struct GetPlayFabIDsFromGameCenterIDsRequest : public PlayFabRequestCommon
        {
            std::list<std::string> GameCenterIDs;

            GetPlayFabIDsFromGameCenterIDsRequest() :
                PlayFabRequestCommon(),
                GameCenterIDs()
            {}

            GetPlayFabIDsFromGameCenterIDsRequest(const GetPlayFabIDsFromGameCenterIDsRequest& src) :
                PlayFabRequestCommon(),
                GameCenterIDs(src.GameCenterIDs)
            {}

            ~GetPlayFabIDsFromGameCenterIDsRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["GameCenterIDs"], GameCenterIDs);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_GameCenterIDs; ToJsonUtilS(GameCenterIDs, each_GameCenterIDs); output["GameCenterIDs"] = each_GameCenterIDs;
                return output;
            }
        };

        struct GetPlayFabIDsFromGameCenterIDsResult : public PlayFabResultCommon
        {
            std::list<GameCenterPlayFabIdPair> Data;

            GetPlayFabIDsFromGameCenterIDsResult() :
                PlayFabResultCommon(),
                Data()
            {}

            GetPlayFabIDsFromGameCenterIDsResult(const GetPlayFabIDsFromGameCenterIDsResult& src) :
                PlayFabResultCommon(),
                Data(src.Data)
            {}

            ~GetPlayFabIDsFromGameCenterIDsResult() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilO(input["Data"], Data);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_Data; ToJsonUtilO(Data, each_Data); output["Data"] = each_Data;
                return output;
            }
        };

        struct GetPlayFabIDsFromGenericIDsRequest : public PlayFabRequestCommon
        {
            std::list<GenericServiceId> GenericIDs;

            GetPlayFabIDsFromGenericIDsRequest() :
                PlayFabRequestCommon(),
                GenericIDs()
            {}

            GetPlayFabIDsFromGenericIDsRequest(const GetPlayFabIDsFromGenericIDsRequest& src) :
                PlayFabRequestCommon(),
                GenericIDs(src.GenericIDs)
            {}

            ~GetPlayFabIDsFromGenericIDsRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilO(input["GenericIDs"], GenericIDs);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_GenericIDs; ToJsonUtilO(GenericIDs, each_GenericIDs); output["GenericIDs"] = each_GenericIDs;
                return output;
            }
        };

        struct GetPlayFabIDsFromGenericIDsResult : public PlayFabResultCommon
        {
            std::list<GenericPlayFabIdPair> Data;

            GetPlayFabIDsFromGenericIDsResult() :
                PlayFabResultCommon(),
                Data()
            {}

            GetPlayFabIDsFromGenericIDsResult(const GetPlayFabIDsFromGenericIDsResult& src) :
                PlayFabResultCommon(),
                Data(src.Data)
            {}

            ~GetPlayFabIDsFromGenericIDsResult() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilO(input["Data"], Data);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_Data; ToJsonUtilO(Data, each_Data); output["Data"] = each_Data;
                return output;
            }
        };

        struct GetPlayFabIDsFromGoogleIDsRequest : public PlayFabRequestCommon
        {
            std::list<std::string> GoogleIDs;

            GetPlayFabIDsFromGoogleIDsRequest() :
                PlayFabRequestCommon(),
                GoogleIDs()
            {}

            GetPlayFabIDsFromGoogleIDsRequest(const GetPlayFabIDsFromGoogleIDsRequest& src) :
                PlayFabRequestCommon(),
                GoogleIDs(src.GoogleIDs)
            {}

            ~GetPlayFabIDsFromGoogleIDsRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["GoogleIDs"], GoogleIDs);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_GoogleIDs; ToJsonUtilS(GoogleIDs, each_GoogleIDs); output["GoogleIDs"] = each_GoogleIDs;
                return output;
            }
        };

        struct GooglePlayFabIdPair : public PlayFabBaseModel
        {
            std::string GoogleId;
            std::string PlayFabId;

            GooglePlayFabIdPair() :
                PlayFabBaseModel(),
                GoogleId(),
                PlayFabId()
            {}

            GooglePlayFabIdPair(const GooglePlayFabIdPair& src) :
                PlayFabBaseModel(),
                GoogleId(src.GoogleId),
                PlayFabId(src.PlayFabId)
            {}

            ~GooglePlayFabIdPair() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["GoogleId"], GoogleId);
                FromJsonUtilS(input["PlayFabId"], PlayFabId);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_GoogleId; ToJsonUtilS(GoogleId, each_GoogleId); output["GoogleId"] = each_GoogleId;
                Json::Value each_PlayFabId; ToJsonUtilS(PlayFabId, each_PlayFabId); output["PlayFabId"] = each_PlayFabId;
                return output;
            }
        };

        struct GetPlayFabIDsFromGoogleIDsResult : public PlayFabResultCommon
        {
            std::list<GooglePlayFabIdPair> Data;

            GetPlayFabIDsFromGoogleIDsResult() :
                PlayFabResultCommon(),
                Data()
            {}

            GetPlayFabIDsFromGoogleIDsResult(const GetPlayFabIDsFromGoogleIDsResult& src) :
                PlayFabResultCommon(),
                Data(src.Data)
            {}

            ~GetPlayFabIDsFromGoogleIDsResult() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilO(input["Data"], Data);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_Data; ToJsonUtilO(Data, each_Data); output["Data"] = each_Data;
                return output;
            }
        };

        struct GetPlayFabIDsFromKongregateIDsRequest : public PlayFabRequestCommon
        {
            std::list<std::string> KongregateIDs;

            GetPlayFabIDsFromKongregateIDsRequest() :
                PlayFabRequestCommon(),
                KongregateIDs()
            {}

            GetPlayFabIDsFromKongregateIDsRequest(const GetPlayFabIDsFromKongregateIDsRequest& src) :
                PlayFabRequestCommon(),
                KongregateIDs(src.KongregateIDs)
            {}

            ~GetPlayFabIDsFromKongregateIDsRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["KongregateIDs"], KongregateIDs);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_KongregateIDs; ToJsonUtilS(KongregateIDs, each_KongregateIDs); output["KongregateIDs"] = each_KongregateIDs;
                return output;
            }
        };

        struct KongregatePlayFabIdPair : public PlayFabBaseModel
        {
            std::string KongregateId;
            std::string PlayFabId;

            KongregatePlayFabIdPair() :
                PlayFabBaseModel(),
                KongregateId(),
                PlayFabId()
            {}

            KongregatePlayFabIdPair(const KongregatePlayFabIdPair& src) :
                PlayFabBaseModel(),
                KongregateId(src.KongregateId),
                PlayFabId(src.PlayFabId)
            {}

            ~KongregatePlayFabIdPair() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["KongregateId"], KongregateId);
                FromJsonUtilS(input["PlayFabId"], PlayFabId);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_KongregateId; ToJsonUtilS(KongregateId, each_KongregateId); output["KongregateId"] = each_KongregateId;
                Json::Value each_PlayFabId; ToJsonUtilS(PlayFabId, each_PlayFabId); output["PlayFabId"] = each_PlayFabId;
                return output;
            }
        };

        struct GetPlayFabIDsFromKongregateIDsResult : public PlayFabResultCommon
        {
            std::list<KongregatePlayFabIdPair> Data;

            GetPlayFabIDsFromKongregateIDsResult() :
                PlayFabResultCommon(),
                Data()
            {}

            GetPlayFabIDsFromKongregateIDsResult(const GetPlayFabIDsFromKongregateIDsResult& src) :
                PlayFabResultCommon(),
                Data(src.Data)
            {}

            ~GetPlayFabIDsFromKongregateIDsResult() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilO(input["Data"], Data);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_Data; ToJsonUtilO(Data, each_Data); output["Data"] = each_Data;
                return output;
            }
        };

        struct GetPlayFabIDsFromNintendoSwitchDeviceIdsRequest : public PlayFabRequestCommon
        {
            std::list<std::string> NintendoSwitchDeviceIds;

            GetPlayFabIDsFromNintendoSwitchDeviceIdsRequest() :
                PlayFabRequestCommon(),
                NintendoSwitchDeviceIds()
            {}

            GetPlayFabIDsFromNintendoSwitchDeviceIdsRequest(const GetPlayFabIDsFromNintendoSwitchDeviceIdsRequest& src) :
                PlayFabRequestCommon(),
                NintendoSwitchDeviceIds(src.NintendoSwitchDeviceIds)
            {}

            ~GetPlayFabIDsFromNintendoSwitchDeviceIdsRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["NintendoSwitchDeviceIds"], NintendoSwitchDeviceIds);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_NintendoSwitchDeviceIds; ToJsonUtilS(NintendoSwitchDeviceIds, each_NintendoSwitchDeviceIds); output["NintendoSwitchDeviceIds"] = each_NintendoSwitchDeviceIds;
                return output;
            }
        };

        struct NintendoSwitchPlayFabIdPair : public PlayFabBaseModel
        {
            std::string NintendoSwitchDeviceId;
            std::string PlayFabId;

            NintendoSwitchPlayFabIdPair() :
                PlayFabBaseModel(),
                NintendoSwitchDeviceId(),
                PlayFabId()
            {}

            NintendoSwitchPlayFabIdPair(const NintendoSwitchPlayFabIdPair& src) :
                PlayFabBaseModel(),
                NintendoSwitchDeviceId(src.NintendoSwitchDeviceId),
                PlayFabId(src.PlayFabId)
            {}

            ~NintendoSwitchPlayFabIdPair() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["NintendoSwitchDeviceId"], NintendoSwitchDeviceId);
                FromJsonUtilS(input["PlayFabId"], PlayFabId);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_NintendoSwitchDeviceId; ToJsonUtilS(NintendoSwitchDeviceId, each_NintendoSwitchDeviceId); output["NintendoSwitchDeviceId"] = each_NintendoSwitchDeviceId;
                Json::Value each_PlayFabId; ToJsonUtilS(PlayFabId, each_PlayFabId); output["PlayFabId"] = each_PlayFabId;
                return output;
            }
        };

        struct GetPlayFabIDsFromNintendoSwitchDeviceIdsResult : public PlayFabResultCommon
        {
            std::list<NintendoSwitchPlayFabIdPair> Data;

            GetPlayFabIDsFromNintendoSwitchDeviceIdsResult() :
                PlayFabResultCommon(),
                Data()
            {}

            GetPlayFabIDsFromNintendoSwitchDeviceIdsResult(const GetPlayFabIDsFromNintendoSwitchDeviceIdsResult& src) :
                PlayFabResultCommon(),
                Data(src.Data)
            {}

            ~GetPlayFabIDsFromNintendoSwitchDeviceIdsResult() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilO(input["Data"], Data);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_Data; ToJsonUtilO(Data, each_Data); output["Data"] = each_Data;
                return output;
            }
        };

        struct GetPlayFabIDsFromPSNAccountIDsRequest : public PlayFabRequestCommon
        {
            Boxed<Int32> IssuerId;
            std::list<std::string> PSNAccountIDs;

            GetPlayFabIDsFromPSNAccountIDsRequest() :
                PlayFabRequestCommon(),
                IssuerId(),
                PSNAccountIDs()
            {}

            GetPlayFabIDsFromPSNAccountIDsRequest(const GetPlayFabIDsFromPSNAccountIDsRequest& src) :
                PlayFabRequestCommon(),
                IssuerId(src.IssuerId),
                PSNAccountIDs(src.PSNAccountIDs)
            {}

            ~GetPlayFabIDsFromPSNAccountIDsRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilP(input["IssuerId"], IssuerId);
                FromJsonUtilS(input["PSNAccountIDs"], PSNAccountIDs);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_IssuerId; ToJsonUtilP(IssuerId, each_IssuerId); output["IssuerId"] = each_IssuerId;
                Json::Value each_PSNAccountIDs; ToJsonUtilS(PSNAccountIDs, each_PSNAccountIDs); output["PSNAccountIDs"] = each_PSNAccountIDs;
                return output;
            }
        };

        struct PSNAccountPlayFabIdPair : public PlayFabBaseModel
        {
            std::string PlayFabId;
            std::string PSNAccountId;

            PSNAccountPlayFabIdPair() :
                PlayFabBaseModel(),
                PlayFabId(),
                PSNAccountId()
            {}

            PSNAccountPlayFabIdPair(const PSNAccountPlayFabIdPair& src) :
                PlayFabBaseModel(),
                PlayFabId(src.PlayFabId),
                PSNAccountId(src.PSNAccountId)
            {}

            ~PSNAccountPlayFabIdPair() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["PlayFabId"], PlayFabId);
                FromJsonUtilS(input["PSNAccountId"], PSNAccountId);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_PlayFabId; ToJsonUtilS(PlayFabId, each_PlayFabId); output["PlayFabId"] = each_PlayFabId;
                Json::Value each_PSNAccountId; ToJsonUtilS(PSNAccountId, each_PSNAccountId); output["PSNAccountId"] = each_PSNAccountId;
                return output;
            }
        };

        struct GetPlayFabIDsFromPSNAccountIDsResult : public PlayFabResultCommon
        {
            std::list<PSNAccountPlayFabIdPair> Data;

            GetPlayFabIDsFromPSNAccountIDsResult() :
                PlayFabResultCommon(),
                Data()
            {}

            GetPlayFabIDsFromPSNAccountIDsResult(const GetPlayFabIDsFromPSNAccountIDsResult& src) :
                PlayFabResultCommon(),
                Data(src.Data)
            {}

            ~GetPlayFabIDsFromPSNAccountIDsResult() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilO(input["Data"], Data);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_Data; ToJsonUtilO(Data, each_Data); output["Data"] = each_Data;
                return output;
            }
        };

        struct GetPlayFabIDsFromSteamIDsRequest : public PlayFabRequestCommon
        {
            std::list<std::string> SteamStringIDs;

            GetPlayFabIDsFromSteamIDsRequest() :
                PlayFabRequestCommon(),
                SteamStringIDs()
            {}

            GetPlayFabIDsFromSteamIDsRequest(const GetPlayFabIDsFromSteamIDsRequest& src) :
                PlayFabRequestCommon(),
                SteamStringIDs(src.SteamStringIDs)
            {}

            ~GetPlayFabIDsFromSteamIDsRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["SteamStringIDs"], SteamStringIDs);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_SteamStringIDs; ToJsonUtilS(SteamStringIDs, each_SteamStringIDs); output["SteamStringIDs"] = each_SteamStringIDs;
                return output;
            }
        };

        struct SteamPlayFabIdPair : public PlayFabBaseModel
        {
            std::string PlayFabId;
            std::string SteamStringId;

            SteamPlayFabIdPair() :
                PlayFabBaseModel(),
                PlayFabId(),
                SteamStringId()
            {}

            SteamPlayFabIdPair(const SteamPlayFabIdPair& src) :
                PlayFabBaseModel(),
                PlayFabId(src.PlayFabId),
                SteamStringId(src.SteamStringId)
            {}

            ~SteamPlayFabIdPair() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["PlayFabId"], PlayFabId);
                FromJsonUtilS(input["SteamStringId"], SteamStringId);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_PlayFabId; ToJsonUtilS(PlayFabId, each_PlayFabId); output["PlayFabId"] = each_PlayFabId;
                Json::Value each_SteamStringId; ToJsonUtilS(SteamStringId, each_SteamStringId); output["SteamStringId"] = each_SteamStringId;
                return output;
            }
        };

        struct GetPlayFabIDsFromSteamIDsResult : public PlayFabResultCommon
        {
            std::list<SteamPlayFabIdPair> Data;

            GetPlayFabIDsFromSteamIDsResult() :
                PlayFabResultCommon(),
                Data()
            {}

            GetPlayFabIDsFromSteamIDsResult(const GetPlayFabIDsFromSteamIDsResult& src) :
                PlayFabResultCommon(),
                Data(src.Data)
            {}

            ~GetPlayFabIDsFromSteamIDsResult() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilO(input["Data"], Data);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_Data; ToJsonUtilO(Data, each_Data); output["Data"] = each_Data;
                return output;
            }
        };

        struct GetPlayFabIDsFromTwitchIDsRequest : public PlayFabRequestCommon
        {
            std::list<std::string> TwitchIds;

            GetPlayFabIDsFromTwitchIDsRequest() :
                PlayFabRequestCommon(),
                TwitchIds()
            {}

            GetPlayFabIDsFromTwitchIDsRequest(const GetPlayFabIDsFromTwitchIDsRequest& src) :
                PlayFabRequestCommon(),
                TwitchIds(src.TwitchIds)
            {}

            ~GetPlayFabIDsFromTwitchIDsRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["TwitchIds"], TwitchIds);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_TwitchIds; ToJsonUtilS(TwitchIds, each_TwitchIds); output["TwitchIds"] = each_TwitchIds;
                return output;
            }
        };

        struct TwitchPlayFabIdPair : public PlayFabBaseModel
        {
            std::string PlayFabId;
            std::string TwitchId;

            TwitchPlayFabIdPair() :
                PlayFabBaseModel(),
                PlayFabId(),
                TwitchId()
            {}

            TwitchPlayFabIdPair(const TwitchPlayFabIdPair& src) :
                PlayFabBaseModel(),
                PlayFabId(src.PlayFabId),
                TwitchId(src.TwitchId)
            {}

            ~TwitchPlayFabIdPair() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["PlayFabId"], PlayFabId);
                FromJsonUtilS(input["TwitchId"], TwitchId);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_PlayFabId; ToJsonUtilS(PlayFabId, each_PlayFabId); output["PlayFabId"] = each_PlayFabId;
                Json::Value each_TwitchId; ToJsonUtilS(TwitchId, each_TwitchId); output["TwitchId"] = each_TwitchId;
                return output;
            }
        };

        struct GetPlayFabIDsFromTwitchIDsResult : public PlayFabResultCommon
        {
            std::list<TwitchPlayFabIdPair> Data;

            GetPlayFabIDsFromTwitchIDsResult() :
                PlayFabResultCommon(),
                Data()
            {}

            GetPlayFabIDsFromTwitchIDsResult(const GetPlayFabIDsFromTwitchIDsResult& src) :
                PlayFabResultCommon(),
                Data(src.Data)
            {}

            ~GetPlayFabIDsFromTwitchIDsResult() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilO(input["Data"], Data);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_Data; ToJsonUtilO(Data, each_Data); output["Data"] = each_Data;
                return output;
            }
        };

        struct GetPlayFabIDsFromXboxLiveIDsRequest : public PlayFabRequestCommon
        {
            std::string Sandbox;
            std::list<std::string> XboxLiveAccountIDs;

            GetPlayFabIDsFromXboxLiveIDsRequest() :
                PlayFabRequestCommon(),
                Sandbox(),
                XboxLiveAccountIDs()
            {}

            GetPlayFabIDsFromXboxLiveIDsRequest(const GetPlayFabIDsFromXboxLiveIDsRequest& src) :
                PlayFabRequestCommon(),
                Sandbox(src.Sandbox),
                XboxLiveAccountIDs(src.XboxLiveAccountIDs)
            {}

            ~GetPlayFabIDsFromXboxLiveIDsRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["Sandbox"], Sandbox);
                FromJsonUtilS(input["XboxLiveAccountIDs"], XboxLiveAccountIDs);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_Sandbox; ToJsonUtilS(Sandbox, each_Sandbox); output["Sandbox"] = each_Sandbox;
                Json::Value each_XboxLiveAccountIDs; ToJsonUtilS(XboxLiveAccountIDs, each_XboxLiveAccountIDs); output["XboxLiveAccountIDs"] = each_XboxLiveAccountIDs;
                return output;
            }
        };

        struct XboxLiveAccountPlayFabIdPair : public PlayFabBaseModel
        {
            std::string PlayFabId;
            std::string XboxLiveAccountId;

            XboxLiveAccountPlayFabIdPair() :
                PlayFabBaseModel(),
                PlayFabId(),
                XboxLiveAccountId()
            {}

            XboxLiveAccountPlayFabIdPair(const XboxLiveAccountPlayFabIdPair& src) :
                PlayFabBaseModel(),
                PlayFabId(src.PlayFabId),
                XboxLiveAccountId(src.XboxLiveAccountId)
            {}

            ~XboxLiveAccountPlayFabIdPair() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["PlayFabId"], PlayFabId);
                FromJsonUtilS(input["XboxLiveAccountId"], XboxLiveAccountId);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_PlayFabId; ToJsonUtilS(PlayFabId, each_PlayFabId); output["PlayFabId"] = each_PlayFabId;
                Json::Value each_XboxLiveAccountId; ToJsonUtilS(XboxLiveAccountId, each_XboxLiveAccountId); output["XboxLiveAccountId"] = each_XboxLiveAccountId;
                return output;
            }
        };

        struct GetPlayFabIDsFromXboxLiveIDsResult : public PlayFabResultCommon
        {
            std::list<XboxLiveAccountPlayFabIdPair> Data;

            GetPlayFabIDsFromXboxLiveIDsResult() :
                PlayFabResultCommon(),
                Data()
            {}

            GetPlayFabIDsFromXboxLiveIDsResult(const GetPlayFabIDsFromXboxLiveIDsResult& src) :
                PlayFabResultCommon(),
                Data(src.Data)
            {}

            ~GetPlayFabIDsFromXboxLiveIDsResult() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilO(input["Data"], Data);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_Data; ToJsonUtilO(Data, each_Data); output["Data"] = each_Data;
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

        struct GetPurchaseRequest : public PlayFabRequestCommon
        {
            std::string OrderId;

            GetPurchaseRequest() :
                PlayFabRequestCommon(),
                OrderId()
            {}

            GetPurchaseRequest(const GetPurchaseRequest& src) :
                PlayFabRequestCommon(),
                OrderId(src.OrderId)
            {}

            ~GetPurchaseRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["OrderId"], OrderId);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_OrderId; ToJsonUtilS(OrderId, each_OrderId); output["OrderId"] = each_OrderId;
                return output;
            }
        };

        struct GetPurchaseResult : public PlayFabResultCommon
        {
            std::string OrderId;
            std::string PaymentProvider;
            time_t PurchaseDate;
            std::string TransactionId;
            std::string TransactionStatus;

            GetPurchaseResult() :
                PlayFabResultCommon(),
                OrderId(),
                PaymentProvider(),
                PurchaseDate(),
                TransactionId(),
                TransactionStatus()
            {}

            GetPurchaseResult(const GetPurchaseResult& src) :
                PlayFabResultCommon(),
                OrderId(src.OrderId),
                PaymentProvider(src.PaymentProvider),
                PurchaseDate(src.PurchaseDate),
                TransactionId(src.TransactionId),
                TransactionStatus(src.TransactionStatus)
            {}

            ~GetPurchaseResult() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["OrderId"], OrderId);
                FromJsonUtilS(input["PaymentProvider"], PaymentProvider);
                FromJsonUtilT(input["PurchaseDate"], PurchaseDate);
                FromJsonUtilS(input["TransactionId"], TransactionId);
                FromJsonUtilS(input["TransactionStatus"], TransactionStatus);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_OrderId; ToJsonUtilS(OrderId, each_OrderId); output["OrderId"] = each_OrderId;
                Json::Value each_PaymentProvider; ToJsonUtilS(PaymentProvider, each_PaymentProvider); output["PaymentProvider"] = each_PaymentProvider;
                Json::Value each_PurchaseDate; ToJsonUtilT(PurchaseDate, each_PurchaseDate); output["PurchaseDate"] = each_PurchaseDate;
                Json::Value each_TransactionId; ToJsonUtilS(TransactionId, each_TransactionId); output["TransactionId"] = each_TransactionId;
                Json::Value each_TransactionStatus; ToJsonUtilS(TransactionStatus, each_TransactionStatus); output["TransactionStatus"] = each_TransactionStatus;
                return output;
            }
        };

        struct GetSharedGroupDataRequest : public PlayFabRequestCommon
        {
            Boxed<bool> GetMembers;
            std::list<std::string> Keys;
            std::string SharedGroupId;

            GetSharedGroupDataRequest() :
                PlayFabRequestCommon(),
                GetMembers(),
                Keys(),
                SharedGroupId()
            {}

            GetSharedGroupDataRequest(const GetSharedGroupDataRequest& src) :
                PlayFabRequestCommon(),
                GetMembers(src.GetMembers),
                Keys(src.Keys),
                SharedGroupId(src.SharedGroupId)
            {}

            ~GetSharedGroupDataRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilP(input["GetMembers"], GetMembers);
                FromJsonUtilS(input["Keys"], Keys);
                FromJsonUtilS(input["SharedGroupId"], SharedGroupId);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_GetMembers; ToJsonUtilP(GetMembers, each_GetMembers); output["GetMembers"] = each_GetMembers;
                Json::Value each_Keys; ToJsonUtilS(Keys, each_Keys); output["Keys"] = each_Keys;
                Json::Value each_SharedGroupId; ToJsonUtilS(SharedGroupId, each_SharedGroupId); output["SharedGroupId"] = each_SharedGroupId;
                return output;
            }
        };

        struct SharedGroupDataRecord : public PlayFabBaseModel
        {
            time_t LastUpdated;
            std::string LastUpdatedBy;
            Boxed<UserDataPermission> Permission;
            std::string Value;

            SharedGroupDataRecord() :
                PlayFabBaseModel(),
                LastUpdated(),
                LastUpdatedBy(),
                Permission(),
                Value()
            {}

            SharedGroupDataRecord(const SharedGroupDataRecord& src) :
                PlayFabBaseModel(),
                LastUpdated(src.LastUpdated),
                LastUpdatedBy(src.LastUpdatedBy),
                Permission(src.Permission),
                Value(src.Value)
            {}

            ~SharedGroupDataRecord() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilT(input["LastUpdated"], LastUpdated);
                FromJsonUtilS(input["LastUpdatedBy"], LastUpdatedBy);
                FromJsonUtilE(input["Permission"], Permission);
                FromJsonUtilS(input["Value"], Value);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_LastUpdated; ToJsonUtilT(LastUpdated, each_LastUpdated); output["LastUpdated"] = each_LastUpdated;
                Json::Value each_LastUpdatedBy; ToJsonUtilS(LastUpdatedBy, each_LastUpdatedBy); output["LastUpdatedBy"] = each_LastUpdatedBy;
                Json::Value each_Permission; ToJsonUtilE(Permission, each_Permission); output["Permission"] = each_Permission;
                Json::Value each_Value; ToJsonUtilS(Value, each_Value); output["Value"] = each_Value;
                return output;
            }
        };

        struct GetSharedGroupDataResult : public PlayFabResultCommon
        {
            std::map<std::string, SharedGroupDataRecord> Data;
            std::list<std::string> Members;

            GetSharedGroupDataResult() :
                PlayFabResultCommon(),
                Data(),
                Members()
            {}

            GetSharedGroupDataResult(const GetSharedGroupDataResult& src) :
                PlayFabResultCommon(),
                Data(src.Data),
                Members(src.Members)
            {}

            ~GetSharedGroupDataResult() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilO(input["Data"], Data);
                FromJsonUtilS(input["Members"], Members);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_Data; ToJsonUtilO(Data, each_Data); output["Data"] = each_Data;
                Json::Value each_Members; ToJsonUtilS(Members, each_Members); output["Members"] = each_Members;
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

        struct GetTimeRequest : public PlayFabRequestCommon
        {

            GetTimeRequest() :
                PlayFabRequestCommon()
            {}

            GetTimeRequest(const GetTimeRequest&) :
                PlayFabRequestCommon()
            {}

            ~GetTimeRequest() = default;

            void FromJson(const Json::Value&) override
            {
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                return output;
            }
        };

        struct GetTimeResult : public PlayFabResultCommon
        {
            time_t Time;

            GetTimeResult() :
                PlayFabResultCommon(),
                Time()
            {}

            GetTimeResult(const GetTimeResult& src) :
                PlayFabResultCommon(),
                Time(src.Time)
            {}

            ~GetTimeResult() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilT(input["Time"], Time);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_Time; ToJsonUtilT(Time, each_Time); output["Time"] = each_Time;
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

        struct GetTitleNewsRequest : public PlayFabRequestCommon
        {
            Boxed<Int32> Count;

            GetTitleNewsRequest() :
                PlayFabRequestCommon(),
                Count()
            {}

            GetTitleNewsRequest(const GetTitleNewsRequest& src) :
                PlayFabRequestCommon(),
                Count(src.Count)
            {}

            ~GetTitleNewsRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilP(input["Count"], Count);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_Count; ToJsonUtilP(Count, each_Count); output["Count"] = each_Count;
                return output;
            }
        };

        struct TitleNewsItem : public PlayFabBaseModel
        {
            std::string Body;
            std::string NewsId;
            time_t Timestamp;
            std::string Title;

            TitleNewsItem() :
                PlayFabBaseModel(),
                Body(),
                NewsId(),
                Timestamp(),
                Title()
            {}

            TitleNewsItem(const TitleNewsItem& src) :
                PlayFabBaseModel(),
                Body(src.Body),
                NewsId(src.NewsId),
                Timestamp(src.Timestamp),
                Title(src.Title)
            {}

            ~TitleNewsItem() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["Body"], Body);
                FromJsonUtilS(input["NewsId"], NewsId);
                FromJsonUtilT(input["Timestamp"], Timestamp);
                FromJsonUtilS(input["Title"], Title);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_Body; ToJsonUtilS(Body, each_Body); output["Body"] = each_Body;
                Json::Value each_NewsId; ToJsonUtilS(NewsId, each_NewsId); output["NewsId"] = each_NewsId;
                Json::Value each_Timestamp; ToJsonUtilT(Timestamp, each_Timestamp); output["Timestamp"] = each_Timestamp;
                Json::Value each_Title; ToJsonUtilS(Title, each_Title); output["Title"] = each_Title;
                return output;
            }
        };

        struct GetTitleNewsResult : public PlayFabResultCommon
        {
            std::list<TitleNewsItem> News;

            GetTitleNewsResult() :
                PlayFabResultCommon(),
                News()
            {}

            GetTitleNewsResult(const GetTitleNewsResult& src) :
                PlayFabResultCommon(),
                News(src.News)
            {}

            ~GetTitleNewsResult() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilO(input["News"], News);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_News; ToJsonUtilO(News, each_News); output["News"] = each_News;
                return output;
            }
        };

        struct GetTitlePublicKeyRequest : public PlayFabRequestCommon
        {
            std::string TitleId;
            std::string TitleSharedSecret;

            GetTitlePublicKeyRequest() :
                PlayFabRequestCommon(),
                TitleId(),
                TitleSharedSecret()
            {}

            GetTitlePublicKeyRequest(const GetTitlePublicKeyRequest& src) :
                PlayFabRequestCommon(),
                TitleId(src.TitleId),
                TitleSharedSecret(src.TitleSharedSecret)
            {}

            ~GetTitlePublicKeyRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["TitleId"], TitleId);
                FromJsonUtilS(input["TitleSharedSecret"], TitleSharedSecret);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_TitleId; ToJsonUtilS(TitleId, each_TitleId); output["TitleId"] = each_TitleId;
                Json::Value each_TitleSharedSecret; ToJsonUtilS(TitleSharedSecret, each_TitleSharedSecret); output["TitleSharedSecret"] = each_TitleSharedSecret;
                return output;
            }
        };

        struct GetTitlePublicKeyResult : public PlayFabResultCommon
        {
            std::string RSAPublicKey;

            GetTitlePublicKeyResult() :
                PlayFabResultCommon(),
                RSAPublicKey()
            {}

            GetTitlePublicKeyResult(const GetTitlePublicKeyResult& src) :
                PlayFabResultCommon(),
                RSAPublicKey(src.RSAPublicKey)
            {}

            ~GetTitlePublicKeyResult() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["RSAPublicKey"], RSAPublicKey);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_RSAPublicKey; ToJsonUtilS(RSAPublicKey, each_RSAPublicKey); output["RSAPublicKey"] = each_RSAPublicKey;
                return output;
            }
        };

        struct GetTradeStatusRequest : public PlayFabRequestCommon
        {
            std::string OfferingPlayerId;
            std::string TradeId;

            GetTradeStatusRequest() :
                PlayFabRequestCommon(),
                OfferingPlayerId(),
                TradeId()
            {}

            GetTradeStatusRequest(const GetTradeStatusRequest& src) :
                PlayFabRequestCommon(),
                OfferingPlayerId(src.OfferingPlayerId),
                TradeId(src.TradeId)
            {}

            ~GetTradeStatusRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["OfferingPlayerId"], OfferingPlayerId);
                FromJsonUtilS(input["TradeId"], TradeId);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_OfferingPlayerId; ToJsonUtilS(OfferingPlayerId, each_OfferingPlayerId); output["OfferingPlayerId"] = each_OfferingPlayerId;
                Json::Value each_TradeId; ToJsonUtilS(TradeId, each_TradeId); output["TradeId"] = each_TradeId;
                return output;
            }
        };

        struct GetTradeStatusResponse : public PlayFabResultCommon
        {
            Boxed<TradeInfo> Trade;

            GetTradeStatusResponse() :
                PlayFabResultCommon(),
                Trade()
            {}

            GetTradeStatusResponse(const GetTradeStatusResponse& src) :
                PlayFabResultCommon(),
                Trade(src.Trade)
            {}

            ~GetTradeStatusResponse() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilO(input["Trade"], Trade);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_Trade; ToJsonUtilO(Trade, each_Trade); output["Trade"] = each_Trade;
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

        struct GetUserDataResult : public PlayFabResultCommon
        {
            std::map<std::string, UserDataRecord> Data;
            Uint32 DataVersion;

            GetUserDataResult() :
                PlayFabResultCommon(),
                Data(),
                DataVersion()
            {}

            GetUserDataResult(const GetUserDataResult& src) :
                PlayFabResultCommon(),
                Data(src.Data),
                DataVersion(src.DataVersion)
            {}

            ~GetUserDataResult() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilO(input["Data"], Data);
                FromJsonUtilP(input["DataVersion"], DataVersion);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_Data; ToJsonUtilO(Data, each_Data); output["Data"] = each_Data;
                Json::Value each_DataVersion; ToJsonUtilP(DataVersion, each_DataVersion); output["DataVersion"] = each_DataVersion;
                return output;
            }
        };

        struct GetUserInventoryRequest : public PlayFabRequestCommon
        {
            std::map<std::string, std::string> CustomTags;

            GetUserInventoryRequest() :
                PlayFabRequestCommon(),
                CustomTags()
            {}

            GetUserInventoryRequest(const GetUserInventoryRequest& src) :
                PlayFabRequestCommon(),
                CustomTags(src.CustomTags)
            {}

            ~GetUserInventoryRequest() = default;

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

        struct GetUserInventoryResult : public PlayFabResultCommon
        {
            std::list<ItemInstance> Inventory;
            std::map<std::string, Int32> VirtualCurrency;
            std::map<std::string, VirtualCurrencyRechargeTime> VirtualCurrencyRechargeTimes;

            GetUserInventoryResult() :
                PlayFabResultCommon(),
                Inventory(),
                VirtualCurrency(),
                VirtualCurrencyRechargeTimes()
            {}

            GetUserInventoryResult(const GetUserInventoryResult& src) :
                PlayFabResultCommon(),
                Inventory(src.Inventory),
                VirtualCurrency(src.VirtualCurrency),
                VirtualCurrencyRechargeTimes(src.VirtualCurrencyRechargeTimes)
            {}

            ~GetUserInventoryResult() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilO(input["Inventory"], Inventory);
                FromJsonUtilP(input["VirtualCurrency"], VirtualCurrency);
                FromJsonUtilO(input["VirtualCurrencyRechargeTimes"], VirtualCurrencyRechargeTimes);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_Inventory; ToJsonUtilO(Inventory, each_Inventory); output["Inventory"] = each_Inventory;
                Json::Value each_VirtualCurrency; ToJsonUtilP(VirtualCurrency, each_VirtualCurrency); output["VirtualCurrency"] = each_VirtualCurrency;
                Json::Value each_VirtualCurrencyRechargeTimes; ToJsonUtilO(VirtualCurrencyRechargeTimes, each_VirtualCurrencyRechargeTimes); output["VirtualCurrencyRechargeTimes"] = each_VirtualCurrencyRechargeTimes;
                return output;
            }
        };

        struct GrantCharacterToUserRequest : public PlayFabRequestCommon
        {
            std::string CatalogVersion;
            std::string CharacterName;
            std::map<std::string, std::string> CustomTags;
            std::string ItemId;

            GrantCharacterToUserRequest() :
                PlayFabRequestCommon(),
                CatalogVersion(),
                CharacterName(),
                CustomTags(),
                ItemId()
            {}

            GrantCharacterToUserRequest(const GrantCharacterToUserRequest& src) :
                PlayFabRequestCommon(),
                CatalogVersion(src.CatalogVersion),
                CharacterName(src.CharacterName),
                CustomTags(src.CustomTags),
                ItemId(src.ItemId)
            {}

            ~GrantCharacterToUserRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["CatalogVersion"], CatalogVersion);
                FromJsonUtilS(input["CharacterName"], CharacterName);
                FromJsonUtilS(input["CustomTags"], CustomTags);
                FromJsonUtilS(input["ItemId"], ItemId);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_CatalogVersion; ToJsonUtilS(CatalogVersion, each_CatalogVersion); output["CatalogVersion"] = each_CatalogVersion;
                Json::Value each_CharacterName; ToJsonUtilS(CharacterName, each_CharacterName); output["CharacterName"] = each_CharacterName;
                Json::Value each_CustomTags; ToJsonUtilS(CustomTags, each_CustomTags); output["CustomTags"] = each_CustomTags;
                Json::Value each_ItemId; ToJsonUtilS(ItemId, each_ItemId); output["ItemId"] = each_ItemId;
                return output;
            }
        };

        struct GrantCharacterToUserResult : public PlayFabResultCommon
        {
            std::string CharacterId;
            std::string CharacterType;
            bool Result;

            GrantCharacterToUserResult() :
                PlayFabResultCommon(),
                CharacterId(),
                CharacterType(),
                Result()
            {}

            GrantCharacterToUserResult(const GrantCharacterToUserResult& src) :
                PlayFabResultCommon(),
                CharacterId(src.CharacterId),
                CharacterType(src.CharacterType),
                Result(src.Result)
            {}

            ~GrantCharacterToUserResult() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["CharacterId"], CharacterId);
                FromJsonUtilS(input["CharacterType"], CharacterType);
                FromJsonUtilP(input["Result"], Result);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_CharacterId; ToJsonUtilS(CharacterId, each_CharacterId); output["CharacterId"] = each_CharacterId;
                Json::Value each_CharacterType; ToJsonUtilS(CharacterType, each_CharacterType); output["CharacterType"] = each_CharacterType;
                Json::Value each_Result; ToJsonUtilP(Result, each_Result); output["Result"] = each_Result;
                return output;
            }
        };

        struct ItemPurchaseRequest : public PlayFabRequestCommon
        {
            std::string Annotation;
            std::string ItemId;
            Uint32 Quantity;
            std::list<std::string> UpgradeFromItems;

            ItemPurchaseRequest() :
                PlayFabRequestCommon(),
                Annotation(),
                ItemId(),
                Quantity(),
                UpgradeFromItems()
            {}

            ItemPurchaseRequest(const ItemPurchaseRequest& src) :
                PlayFabRequestCommon(),
                Annotation(src.Annotation),
                ItemId(src.ItemId),
                Quantity(src.Quantity),
                UpgradeFromItems(src.UpgradeFromItems)
            {}

            ~ItemPurchaseRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["Annotation"], Annotation);
                FromJsonUtilS(input["ItemId"], ItemId);
                FromJsonUtilP(input["Quantity"], Quantity);
                FromJsonUtilS(input["UpgradeFromItems"], UpgradeFromItems);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_Annotation; ToJsonUtilS(Annotation, each_Annotation); output["Annotation"] = each_Annotation;
                Json::Value each_ItemId; ToJsonUtilS(ItemId, each_ItemId); output["ItemId"] = each_ItemId;
                Json::Value each_Quantity; ToJsonUtilP(Quantity, each_Quantity); output["Quantity"] = each_Quantity;
                Json::Value each_UpgradeFromItems; ToJsonUtilS(UpgradeFromItems, each_UpgradeFromItems); output["UpgradeFromItems"] = each_UpgradeFromItems;
                return output;
            }
        };

        struct LinkAndroidDeviceIDRequest : public PlayFabRequestCommon
        {
            std::string AndroidDevice;
            std::string AndroidDeviceId;
            std::map<std::string, std::string> CustomTags;
            Boxed<bool> ForceLink;
            std::string OS;

            LinkAndroidDeviceIDRequest() :
                PlayFabRequestCommon(),
                AndroidDevice(),
                AndroidDeviceId(),
                CustomTags(),
                ForceLink(),
                OS()
            {}

            LinkAndroidDeviceIDRequest(const LinkAndroidDeviceIDRequest& src) :
                PlayFabRequestCommon(),
                AndroidDevice(src.AndroidDevice),
                AndroidDeviceId(src.AndroidDeviceId),
                CustomTags(src.CustomTags),
                ForceLink(src.ForceLink),
                OS(src.OS)
            {}

            ~LinkAndroidDeviceIDRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["AndroidDevice"], AndroidDevice);
                FromJsonUtilS(input["AndroidDeviceId"], AndroidDeviceId);
                FromJsonUtilS(input["CustomTags"], CustomTags);
                FromJsonUtilP(input["ForceLink"], ForceLink);
                FromJsonUtilS(input["OS"], OS);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_AndroidDevice; ToJsonUtilS(AndroidDevice, each_AndroidDevice); output["AndroidDevice"] = each_AndroidDevice;
                Json::Value each_AndroidDeviceId; ToJsonUtilS(AndroidDeviceId, each_AndroidDeviceId); output["AndroidDeviceId"] = each_AndroidDeviceId;
                Json::Value each_CustomTags; ToJsonUtilS(CustomTags, each_CustomTags); output["CustomTags"] = each_CustomTags;
                Json::Value each_ForceLink; ToJsonUtilP(ForceLink, each_ForceLink); output["ForceLink"] = each_ForceLink;
                Json::Value each_OS; ToJsonUtilS(OS, each_OS); output["OS"] = each_OS;
                return output;
            }
        };

        struct LinkAndroidDeviceIDResult : public PlayFabResultCommon
        {

            LinkAndroidDeviceIDResult() :
                PlayFabResultCommon()
            {}

            LinkAndroidDeviceIDResult(const LinkAndroidDeviceIDResult&) :
                PlayFabResultCommon()
            {}

            ~LinkAndroidDeviceIDResult() = default;

            void FromJson(const Json::Value&) override
            {
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                return output;
            }
        };

        struct LinkAppleRequest : public PlayFabRequestCommon
        {
            std::map<std::string, std::string> CustomTags;
            Boxed<bool> ForceLink;
            std::string IdentityToken;

            LinkAppleRequest() :
                PlayFabRequestCommon(),
                CustomTags(),
                ForceLink(),
                IdentityToken()
            {}

            LinkAppleRequest(const LinkAppleRequest& src) :
                PlayFabRequestCommon(),
                CustomTags(src.CustomTags),
                ForceLink(src.ForceLink),
                IdentityToken(src.IdentityToken)
            {}

            ~LinkAppleRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["CustomTags"], CustomTags);
                FromJsonUtilP(input["ForceLink"], ForceLink);
                FromJsonUtilS(input["IdentityToken"], IdentityToken);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_CustomTags; ToJsonUtilS(CustomTags, each_CustomTags); output["CustomTags"] = each_CustomTags;
                Json::Value each_ForceLink; ToJsonUtilP(ForceLink, each_ForceLink); output["ForceLink"] = each_ForceLink;
                Json::Value each_IdentityToken; ToJsonUtilS(IdentityToken, each_IdentityToken); output["IdentityToken"] = each_IdentityToken;
                return output;
            }
        };

        struct LinkCustomIDRequest : public PlayFabRequestCommon
        {
            std::string CustomId;
            std::map<std::string, std::string> CustomTags;
            Boxed<bool> ForceLink;

            LinkCustomIDRequest() :
                PlayFabRequestCommon(),
                CustomId(),
                CustomTags(),
                ForceLink()
            {}

            LinkCustomIDRequest(const LinkCustomIDRequest& src) :
                PlayFabRequestCommon(),
                CustomId(src.CustomId),
                CustomTags(src.CustomTags),
                ForceLink(src.ForceLink)
            {}

            ~LinkCustomIDRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["CustomId"], CustomId);
                FromJsonUtilS(input["CustomTags"], CustomTags);
                FromJsonUtilP(input["ForceLink"], ForceLink);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_CustomId; ToJsonUtilS(CustomId, each_CustomId); output["CustomId"] = each_CustomId;
                Json::Value each_CustomTags; ToJsonUtilS(CustomTags, each_CustomTags); output["CustomTags"] = each_CustomTags;
                Json::Value each_ForceLink; ToJsonUtilP(ForceLink, each_ForceLink); output["ForceLink"] = each_ForceLink;
                return output;
            }
        };

        struct LinkCustomIDResult : public PlayFabResultCommon
        {

            LinkCustomIDResult() :
                PlayFabResultCommon()
            {}

            LinkCustomIDResult(const LinkCustomIDResult&) :
                PlayFabResultCommon()
            {}

            ~LinkCustomIDResult() = default;

            void FromJson(const Json::Value&) override
            {
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                return output;
            }
        };

        struct LinkFacebookAccountRequest : public PlayFabRequestCommon
        {
            std::string AccessToken;
            std::map<std::string, std::string> CustomTags;
            Boxed<bool> ForceLink;

            LinkFacebookAccountRequest() :
                PlayFabRequestCommon(),
                AccessToken(),
                CustomTags(),
                ForceLink()
            {}

            LinkFacebookAccountRequest(const LinkFacebookAccountRequest& src) :
                PlayFabRequestCommon(),
                AccessToken(src.AccessToken),
                CustomTags(src.CustomTags),
                ForceLink(src.ForceLink)
            {}

            ~LinkFacebookAccountRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["AccessToken"], AccessToken);
                FromJsonUtilS(input["CustomTags"], CustomTags);
                FromJsonUtilP(input["ForceLink"], ForceLink);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_AccessToken; ToJsonUtilS(AccessToken, each_AccessToken); output["AccessToken"] = each_AccessToken;
                Json::Value each_CustomTags; ToJsonUtilS(CustomTags, each_CustomTags); output["CustomTags"] = each_CustomTags;
                Json::Value each_ForceLink; ToJsonUtilP(ForceLink, each_ForceLink); output["ForceLink"] = each_ForceLink;
                return output;
            }
        };

        struct LinkFacebookAccountResult : public PlayFabResultCommon
        {

            LinkFacebookAccountResult() :
                PlayFabResultCommon()
            {}

            LinkFacebookAccountResult(const LinkFacebookAccountResult&) :
                PlayFabResultCommon()
            {}

            ~LinkFacebookAccountResult() = default;

            void FromJson(const Json::Value&) override
            {
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                return output;
            }
        };

        struct LinkFacebookInstantGamesIdRequest : public PlayFabRequestCommon
        {
            std::map<std::string, std::string> CustomTags;
            std::string FacebookInstantGamesSignature;
            Boxed<bool> ForceLink;

            LinkFacebookInstantGamesIdRequest() :
                PlayFabRequestCommon(),
                CustomTags(),
                FacebookInstantGamesSignature(),
                ForceLink()
            {}

            LinkFacebookInstantGamesIdRequest(const LinkFacebookInstantGamesIdRequest& src) :
                PlayFabRequestCommon(),
                CustomTags(src.CustomTags),
                FacebookInstantGamesSignature(src.FacebookInstantGamesSignature),
                ForceLink(src.ForceLink)
            {}

            ~LinkFacebookInstantGamesIdRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["CustomTags"], CustomTags);
                FromJsonUtilS(input["FacebookInstantGamesSignature"], FacebookInstantGamesSignature);
                FromJsonUtilP(input["ForceLink"], ForceLink);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_CustomTags; ToJsonUtilS(CustomTags, each_CustomTags); output["CustomTags"] = each_CustomTags;
                Json::Value each_FacebookInstantGamesSignature; ToJsonUtilS(FacebookInstantGamesSignature, each_FacebookInstantGamesSignature); output["FacebookInstantGamesSignature"] = each_FacebookInstantGamesSignature;
                Json::Value each_ForceLink; ToJsonUtilP(ForceLink, each_ForceLink); output["ForceLink"] = each_ForceLink;
                return output;
            }
        };

        struct LinkFacebookInstantGamesIdResult : public PlayFabResultCommon
        {

            LinkFacebookInstantGamesIdResult() :
                PlayFabResultCommon()
            {}

            LinkFacebookInstantGamesIdResult(const LinkFacebookInstantGamesIdResult&) :
                PlayFabResultCommon()
            {}

            ~LinkFacebookInstantGamesIdResult() = default;

            void FromJson(const Json::Value&) override
            {
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                return output;
            }
        };

        struct LinkGameCenterAccountRequest : public PlayFabRequestCommon
        {
            std::map<std::string, std::string> CustomTags;
            Boxed<bool> ForceLink;
            std::string GameCenterId;
            std::string PublicKeyUrl;
            std::string Salt;
            std::string Signature;
            std::string Timestamp;

            LinkGameCenterAccountRequest() :
                PlayFabRequestCommon(),
                CustomTags(),
                ForceLink(),
                GameCenterId(),
                PublicKeyUrl(),
                Salt(),
                Signature(),
                Timestamp()
            {}

            LinkGameCenterAccountRequest(const LinkGameCenterAccountRequest& src) :
                PlayFabRequestCommon(),
                CustomTags(src.CustomTags),
                ForceLink(src.ForceLink),
                GameCenterId(src.GameCenterId),
                PublicKeyUrl(src.PublicKeyUrl),
                Salt(src.Salt),
                Signature(src.Signature),
                Timestamp(src.Timestamp)
            {}

            ~LinkGameCenterAccountRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["CustomTags"], CustomTags);
                FromJsonUtilP(input["ForceLink"], ForceLink);
                FromJsonUtilS(input["GameCenterId"], GameCenterId);
                FromJsonUtilS(input["PublicKeyUrl"], PublicKeyUrl);
                FromJsonUtilS(input["Salt"], Salt);
                FromJsonUtilS(input["Signature"], Signature);
                FromJsonUtilS(input["Timestamp"], Timestamp);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_CustomTags; ToJsonUtilS(CustomTags, each_CustomTags); output["CustomTags"] = each_CustomTags;
                Json::Value each_ForceLink; ToJsonUtilP(ForceLink, each_ForceLink); output["ForceLink"] = each_ForceLink;
                Json::Value each_GameCenterId; ToJsonUtilS(GameCenterId, each_GameCenterId); output["GameCenterId"] = each_GameCenterId;
                Json::Value each_PublicKeyUrl; ToJsonUtilS(PublicKeyUrl, each_PublicKeyUrl); output["PublicKeyUrl"] = each_PublicKeyUrl;
                Json::Value each_Salt; ToJsonUtilS(Salt, each_Salt); output["Salt"] = each_Salt;
                Json::Value each_Signature; ToJsonUtilS(Signature, each_Signature); output["Signature"] = each_Signature;
                Json::Value each_Timestamp; ToJsonUtilS(Timestamp, each_Timestamp); output["Timestamp"] = each_Timestamp;
                return output;
            }
        };

        struct LinkGameCenterAccountResult : public PlayFabResultCommon
        {

            LinkGameCenterAccountResult() :
                PlayFabResultCommon()
            {}

            LinkGameCenterAccountResult(const LinkGameCenterAccountResult&) :
                PlayFabResultCommon()
            {}

            ~LinkGameCenterAccountResult() = default;

            void FromJson(const Json::Value&) override
            {
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                return output;
            }
        };

        struct LinkGoogleAccountRequest : public PlayFabRequestCommon
        {
            std::map<std::string, std::string> CustomTags;
            Boxed<bool> ForceLink;
            std::string ServerAuthCode;

            LinkGoogleAccountRequest() :
                PlayFabRequestCommon(),
                CustomTags(),
                ForceLink(),
                ServerAuthCode()
            {}

            LinkGoogleAccountRequest(const LinkGoogleAccountRequest& src) :
                PlayFabRequestCommon(),
                CustomTags(src.CustomTags),
                ForceLink(src.ForceLink),
                ServerAuthCode(src.ServerAuthCode)
            {}

            ~LinkGoogleAccountRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["CustomTags"], CustomTags);
                FromJsonUtilP(input["ForceLink"], ForceLink);
                FromJsonUtilS(input["ServerAuthCode"], ServerAuthCode);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_CustomTags; ToJsonUtilS(CustomTags, each_CustomTags); output["CustomTags"] = each_CustomTags;
                Json::Value each_ForceLink; ToJsonUtilP(ForceLink, each_ForceLink); output["ForceLink"] = each_ForceLink;
                Json::Value each_ServerAuthCode; ToJsonUtilS(ServerAuthCode, each_ServerAuthCode); output["ServerAuthCode"] = each_ServerAuthCode;
                return output;
            }
        };

        struct LinkGoogleAccountResult : public PlayFabResultCommon
        {

            LinkGoogleAccountResult() :
                PlayFabResultCommon()
            {}

            LinkGoogleAccountResult(const LinkGoogleAccountResult&) :
                PlayFabResultCommon()
            {}

            ~LinkGoogleAccountResult() = default;

            void FromJson(const Json::Value&) override
            {
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                return output;
            }
        };

        struct LinkIOSDeviceIDRequest : public PlayFabRequestCommon
        {
            std::map<std::string, std::string> CustomTags;
            std::string DeviceId;
            std::string DeviceModel;
            Boxed<bool> ForceLink;
            std::string OS;

            LinkIOSDeviceIDRequest() :
                PlayFabRequestCommon(),
                CustomTags(),
                DeviceId(),
                DeviceModel(),
                ForceLink(),
                OS()
            {}

            LinkIOSDeviceIDRequest(const LinkIOSDeviceIDRequest& src) :
                PlayFabRequestCommon(),
                CustomTags(src.CustomTags),
                DeviceId(src.DeviceId),
                DeviceModel(src.DeviceModel),
                ForceLink(src.ForceLink),
                OS(src.OS)
            {}

            ~LinkIOSDeviceIDRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["CustomTags"], CustomTags);
                FromJsonUtilS(input["DeviceId"], DeviceId);
                FromJsonUtilS(input["DeviceModel"], DeviceModel);
                FromJsonUtilP(input["ForceLink"], ForceLink);
                FromJsonUtilS(input["OS"], OS);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_CustomTags; ToJsonUtilS(CustomTags, each_CustomTags); output["CustomTags"] = each_CustomTags;
                Json::Value each_DeviceId; ToJsonUtilS(DeviceId, each_DeviceId); output["DeviceId"] = each_DeviceId;
                Json::Value each_DeviceModel; ToJsonUtilS(DeviceModel, each_DeviceModel); output["DeviceModel"] = each_DeviceModel;
                Json::Value each_ForceLink; ToJsonUtilP(ForceLink, each_ForceLink); output["ForceLink"] = each_ForceLink;
                Json::Value each_OS; ToJsonUtilS(OS, each_OS); output["OS"] = each_OS;
                return output;
            }
        };

        struct LinkIOSDeviceIDResult : public PlayFabResultCommon
        {

            LinkIOSDeviceIDResult() :
                PlayFabResultCommon()
            {}

            LinkIOSDeviceIDResult(const LinkIOSDeviceIDResult&) :
                PlayFabResultCommon()
            {}

            ~LinkIOSDeviceIDResult() = default;

            void FromJson(const Json::Value&) override
            {
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                return output;
            }
        };

        struct LinkKongregateAccountRequest : public PlayFabRequestCommon
        {
            std::string AuthTicket;
            std::map<std::string, std::string> CustomTags;
            Boxed<bool> ForceLink;
            std::string KongregateId;

            LinkKongregateAccountRequest() :
                PlayFabRequestCommon(),
                AuthTicket(),
                CustomTags(),
                ForceLink(),
                KongregateId()
            {}

            LinkKongregateAccountRequest(const LinkKongregateAccountRequest& src) :
                PlayFabRequestCommon(),
                AuthTicket(src.AuthTicket),
                CustomTags(src.CustomTags),
                ForceLink(src.ForceLink),
                KongregateId(src.KongregateId)
            {}

            ~LinkKongregateAccountRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["AuthTicket"], AuthTicket);
                FromJsonUtilS(input["CustomTags"], CustomTags);
                FromJsonUtilP(input["ForceLink"], ForceLink);
                FromJsonUtilS(input["KongregateId"], KongregateId);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_AuthTicket; ToJsonUtilS(AuthTicket, each_AuthTicket); output["AuthTicket"] = each_AuthTicket;
                Json::Value each_CustomTags; ToJsonUtilS(CustomTags, each_CustomTags); output["CustomTags"] = each_CustomTags;
                Json::Value each_ForceLink; ToJsonUtilP(ForceLink, each_ForceLink); output["ForceLink"] = each_ForceLink;
                Json::Value each_KongregateId; ToJsonUtilS(KongregateId, each_KongregateId); output["KongregateId"] = each_KongregateId;
                return output;
            }
        };

        struct LinkKongregateAccountResult : public PlayFabResultCommon
        {

            LinkKongregateAccountResult() :
                PlayFabResultCommon()
            {}

            LinkKongregateAccountResult(const LinkKongregateAccountResult&) :
                PlayFabResultCommon()
            {}

            ~LinkKongregateAccountResult() = default;

            void FromJson(const Json::Value&) override
            {
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                return output;
            }
        };

        struct LinkNintendoServiceAccountRequest : public PlayFabRequestCommon
        {
            std::map<std::string, std::string> CustomTags;
            Boxed<bool> ForceLink;
            std::string IdentityToken;

            LinkNintendoServiceAccountRequest() :
                PlayFabRequestCommon(),
                CustomTags(),
                ForceLink(),
                IdentityToken()
            {}

            LinkNintendoServiceAccountRequest(const LinkNintendoServiceAccountRequest& src) :
                PlayFabRequestCommon(),
                CustomTags(src.CustomTags),
                ForceLink(src.ForceLink),
                IdentityToken(src.IdentityToken)
            {}

            ~LinkNintendoServiceAccountRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["CustomTags"], CustomTags);
                FromJsonUtilP(input["ForceLink"], ForceLink);
                FromJsonUtilS(input["IdentityToken"], IdentityToken);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_CustomTags; ToJsonUtilS(CustomTags, each_CustomTags); output["CustomTags"] = each_CustomTags;
                Json::Value each_ForceLink; ToJsonUtilP(ForceLink, each_ForceLink); output["ForceLink"] = each_ForceLink;
                Json::Value each_IdentityToken; ToJsonUtilS(IdentityToken, each_IdentityToken); output["IdentityToken"] = each_IdentityToken;
                return output;
            }
        };

        struct LinkNintendoSwitchDeviceIdRequest : public PlayFabRequestCommon
        {
            std::map<std::string, std::string> CustomTags;
            Boxed<bool> ForceLink;
            std::string NintendoSwitchDeviceId;

            LinkNintendoSwitchDeviceIdRequest() :
                PlayFabRequestCommon(),
                CustomTags(),
                ForceLink(),
                NintendoSwitchDeviceId()
            {}

            LinkNintendoSwitchDeviceIdRequest(const LinkNintendoSwitchDeviceIdRequest& src) :
                PlayFabRequestCommon(),
                CustomTags(src.CustomTags),
                ForceLink(src.ForceLink),
                NintendoSwitchDeviceId(src.NintendoSwitchDeviceId)
            {}

            ~LinkNintendoSwitchDeviceIdRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["CustomTags"], CustomTags);
                FromJsonUtilP(input["ForceLink"], ForceLink);
                FromJsonUtilS(input["NintendoSwitchDeviceId"], NintendoSwitchDeviceId);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_CustomTags; ToJsonUtilS(CustomTags, each_CustomTags); output["CustomTags"] = each_CustomTags;
                Json::Value each_ForceLink; ToJsonUtilP(ForceLink, each_ForceLink); output["ForceLink"] = each_ForceLink;
                Json::Value each_NintendoSwitchDeviceId; ToJsonUtilS(NintendoSwitchDeviceId, each_NintendoSwitchDeviceId); output["NintendoSwitchDeviceId"] = each_NintendoSwitchDeviceId;
                return output;
            }
        };

        struct LinkNintendoSwitchDeviceIdResult : public PlayFabResultCommon
        {

            LinkNintendoSwitchDeviceIdResult() :
                PlayFabResultCommon()
            {}

            LinkNintendoSwitchDeviceIdResult(const LinkNintendoSwitchDeviceIdResult&) :
                PlayFabResultCommon()
            {}

            ~LinkNintendoSwitchDeviceIdResult() = default;

            void FromJson(const Json::Value&) override
            {
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                return output;
            }
        };

        struct LinkOpenIdConnectRequest : public PlayFabRequestCommon
        {
            std::string ConnectionId;
            std::map<std::string, std::string> CustomTags;
            Boxed<bool> ForceLink;
            std::string IdToken;

            LinkOpenIdConnectRequest() :
                PlayFabRequestCommon(),
                ConnectionId(),
                CustomTags(),
                ForceLink(),
                IdToken()
            {}

            LinkOpenIdConnectRequest(const LinkOpenIdConnectRequest& src) :
                PlayFabRequestCommon(),
                ConnectionId(src.ConnectionId),
                CustomTags(src.CustomTags),
                ForceLink(src.ForceLink),
                IdToken(src.IdToken)
            {}

            ~LinkOpenIdConnectRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["ConnectionId"], ConnectionId);
                FromJsonUtilS(input["CustomTags"], CustomTags);
                FromJsonUtilP(input["ForceLink"], ForceLink);
                FromJsonUtilS(input["IdToken"], IdToken);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_ConnectionId; ToJsonUtilS(ConnectionId, each_ConnectionId); output["ConnectionId"] = each_ConnectionId;
                Json::Value each_CustomTags; ToJsonUtilS(CustomTags, each_CustomTags); output["CustomTags"] = each_CustomTags;
                Json::Value each_ForceLink; ToJsonUtilP(ForceLink, each_ForceLink); output["ForceLink"] = each_ForceLink;
                Json::Value each_IdToken; ToJsonUtilS(IdToken, each_IdToken); output["IdToken"] = each_IdToken;
                return output;
            }
        };

        struct LinkPSNAccountRequest : public PlayFabRequestCommon
        {
            std::string AuthCode;
            std::map<std::string, std::string> CustomTags;
            Boxed<bool> ForceLink;
            Boxed<Int32> IssuerId;
            std::string RedirectUri;

            LinkPSNAccountRequest() :
                PlayFabRequestCommon(),
                AuthCode(),
                CustomTags(),
                ForceLink(),
                IssuerId(),
                RedirectUri()
            {}

            LinkPSNAccountRequest(const LinkPSNAccountRequest& src) :
                PlayFabRequestCommon(),
                AuthCode(src.AuthCode),
                CustomTags(src.CustomTags),
                ForceLink(src.ForceLink),
                IssuerId(src.IssuerId),
                RedirectUri(src.RedirectUri)
            {}

            ~LinkPSNAccountRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["AuthCode"], AuthCode);
                FromJsonUtilS(input["CustomTags"], CustomTags);
                FromJsonUtilP(input["ForceLink"], ForceLink);
                FromJsonUtilP(input["IssuerId"], IssuerId);
                FromJsonUtilS(input["RedirectUri"], RedirectUri);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_AuthCode; ToJsonUtilS(AuthCode, each_AuthCode); output["AuthCode"] = each_AuthCode;
                Json::Value each_CustomTags; ToJsonUtilS(CustomTags, each_CustomTags); output["CustomTags"] = each_CustomTags;
                Json::Value each_ForceLink; ToJsonUtilP(ForceLink, each_ForceLink); output["ForceLink"] = each_ForceLink;
                Json::Value each_IssuerId; ToJsonUtilP(IssuerId, each_IssuerId); output["IssuerId"] = each_IssuerId;
                Json::Value each_RedirectUri; ToJsonUtilS(RedirectUri, each_RedirectUri); output["RedirectUri"] = each_RedirectUri;
                return output;
            }
        };

        struct LinkPSNAccountResult : public PlayFabResultCommon
        {

            LinkPSNAccountResult() :
                PlayFabResultCommon()
            {}

            LinkPSNAccountResult(const LinkPSNAccountResult&) :
                PlayFabResultCommon()
            {}

            ~LinkPSNAccountResult() = default;

            void FromJson(const Json::Value&) override
            {
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                return output;
            }
        };

        struct LinkSteamAccountRequest : public PlayFabRequestCommon
        {
            std::map<std::string, std::string> CustomTags;
            Boxed<bool> ForceLink;
            std::string SteamTicket;

            LinkSteamAccountRequest() :
                PlayFabRequestCommon(),
                CustomTags(),
                ForceLink(),
                SteamTicket()
            {}

            LinkSteamAccountRequest(const LinkSteamAccountRequest& src) :
                PlayFabRequestCommon(),
                CustomTags(src.CustomTags),
                ForceLink(src.ForceLink),
                SteamTicket(src.SteamTicket)
            {}

            ~LinkSteamAccountRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["CustomTags"], CustomTags);
                FromJsonUtilP(input["ForceLink"], ForceLink);
                FromJsonUtilS(input["SteamTicket"], SteamTicket);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_CustomTags; ToJsonUtilS(CustomTags, each_CustomTags); output["CustomTags"] = each_CustomTags;
                Json::Value each_ForceLink; ToJsonUtilP(ForceLink, each_ForceLink); output["ForceLink"] = each_ForceLink;
                Json::Value each_SteamTicket; ToJsonUtilS(SteamTicket, each_SteamTicket); output["SteamTicket"] = each_SteamTicket;
                return output;
            }
        };

        struct LinkSteamAccountResult : public PlayFabResultCommon
        {

            LinkSteamAccountResult() :
                PlayFabResultCommon()
            {}

            LinkSteamAccountResult(const LinkSteamAccountResult&) :
                PlayFabResultCommon()
            {}

            ~LinkSteamAccountResult() = default;

            void FromJson(const Json::Value&) override
            {
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                return output;
            }
        };

        struct LinkTwitchAccountRequest : public PlayFabRequestCommon
        {
            std::string AccessToken;
            std::map<std::string, std::string> CustomTags;
            Boxed<bool> ForceLink;

            LinkTwitchAccountRequest() :
                PlayFabRequestCommon(),
                AccessToken(),
                CustomTags(),
                ForceLink()
            {}

            LinkTwitchAccountRequest(const LinkTwitchAccountRequest& src) :
                PlayFabRequestCommon(),
                AccessToken(src.AccessToken),
                CustomTags(src.CustomTags),
                ForceLink(src.ForceLink)
            {}

            ~LinkTwitchAccountRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["AccessToken"], AccessToken);
                FromJsonUtilS(input["CustomTags"], CustomTags);
                FromJsonUtilP(input["ForceLink"], ForceLink);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_AccessToken; ToJsonUtilS(AccessToken, each_AccessToken); output["AccessToken"] = each_AccessToken;
                Json::Value each_CustomTags; ToJsonUtilS(CustomTags, each_CustomTags); output["CustomTags"] = each_CustomTags;
                Json::Value each_ForceLink; ToJsonUtilP(ForceLink, each_ForceLink); output["ForceLink"] = each_ForceLink;
                return output;
            }
        };

        struct LinkTwitchAccountResult : public PlayFabResultCommon
        {

            LinkTwitchAccountResult() :
                PlayFabResultCommon()
            {}

            LinkTwitchAccountResult(const LinkTwitchAccountResult&) :
                PlayFabResultCommon()
            {}

            ~LinkTwitchAccountResult() = default;

            void FromJson(const Json::Value&) override
            {
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                return output;
            }
        };

        struct LinkXboxAccountRequest : public PlayFabRequestCommon
        {
            std::map<std::string, std::string> CustomTags;
            Boxed<bool> ForceLink;
            std::string XboxToken;

            LinkXboxAccountRequest() :
                PlayFabRequestCommon(),
                CustomTags(),
                ForceLink(),
                XboxToken()
            {}

            LinkXboxAccountRequest(const LinkXboxAccountRequest& src) :
                PlayFabRequestCommon(),
                CustomTags(src.CustomTags),
                ForceLink(src.ForceLink),
                XboxToken(src.XboxToken)
            {}

            ~LinkXboxAccountRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["CustomTags"], CustomTags);
                FromJsonUtilP(input["ForceLink"], ForceLink);
                FromJsonUtilS(input["XboxToken"], XboxToken);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_CustomTags; ToJsonUtilS(CustomTags, each_CustomTags); output["CustomTags"] = each_CustomTags;
                Json::Value each_ForceLink; ToJsonUtilP(ForceLink, each_ForceLink); output["ForceLink"] = each_ForceLink;
                Json::Value each_XboxToken; ToJsonUtilS(XboxToken, each_XboxToken); output["XboxToken"] = each_XboxToken;
                return output;
            }
        };

        struct LinkXboxAccountResult : public PlayFabResultCommon
        {

            LinkXboxAccountResult() :
                PlayFabResultCommon()
            {}

            LinkXboxAccountResult(const LinkXboxAccountResult&) :
                PlayFabResultCommon()
            {}

            ~LinkXboxAccountResult() = default;

            void FromJson(const Json::Value&) override
            {
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                return output;
            }
        };

        struct ListUsersCharactersRequest : public PlayFabRequestCommon
        {
            std::string PlayFabId;

            ListUsersCharactersRequest() :
                PlayFabRequestCommon(),
                PlayFabId()
            {}

            ListUsersCharactersRequest(const ListUsersCharactersRequest& src) :
                PlayFabRequestCommon(),
                PlayFabId(src.PlayFabId)
            {}

            ~ListUsersCharactersRequest() = default;

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

        struct ListUsersCharactersResult : public PlayFabResultCommon
        {
            std::list<CharacterResult> Characters;

            ListUsersCharactersResult() :
                PlayFabResultCommon(),
                Characters()
            {}

            ListUsersCharactersResult(const ListUsersCharactersResult& src) :
                PlayFabResultCommon(),
                Characters(src.Characters)
            {}

            ~ListUsersCharactersResult() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilO(input["Characters"], Characters);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_Characters; ToJsonUtilO(Characters, each_Characters); output["Characters"] = each_Characters;
                return output;
            }
        };

        struct UserSettings : public PlayFabBaseModel
        {
            bool GatherDeviceInfo;
            bool GatherFocusInfo;
            bool NeedsAttribution;

            UserSettings() :
                PlayFabBaseModel(),
                GatherDeviceInfo(),
                GatherFocusInfo(),
                NeedsAttribution()
            {}

            UserSettings(const UserSettings& src) :
                PlayFabBaseModel(),
                GatherDeviceInfo(src.GatherDeviceInfo),
                GatherFocusInfo(src.GatherFocusInfo),
                NeedsAttribution(src.NeedsAttribution)
            {}

            ~UserSettings() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilP(input["GatherDeviceInfo"], GatherDeviceInfo);
                FromJsonUtilP(input["GatherFocusInfo"], GatherFocusInfo);
                FromJsonUtilP(input["NeedsAttribution"], NeedsAttribution);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_GatherDeviceInfo; ToJsonUtilP(GatherDeviceInfo, each_GatherDeviceInfo); output["GatherDeviceInfo"] = each_GatherDeviceInfo;
                Json::Value each_GatherFocusInfo; ToJsonUtilP(GatherFocusInfo, each_GatherFocusInfo); output["GatherFocusInfo"] = each_GatherFocusInfo;
                Json::Value each_NeedsAttribution; ToJsonUtilP(NeedsAttribution, each_NeedsAttribution); output["NeedsAttribution"] = each_NeedsAttribution;
                return output;
            }
        };

        struct Variable : public PlayFabBaseModel
        {
            std::string Name;
            std::string Value;

            Variable() :
                PlayFabBaseModel(),
                Name(),
                Value()
            {}

            Variable(const Variable& src) :
                PlayFabBaseModel(),
                Name(src.Name),
                Value(src.Value)
            {}

            ~Variable() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["Name"], Name);
                FromJsonUtilS(input["Value"], Value);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_Name; ToJsonUtilS(Name, each_Name); output["Name"] = each_Name;
                Json::Value each_Value; ToJsonUtilS(Value, each_Value); output["Value"] = each_Value;
                return output;
            }
        };

        struct TreatmentAssignment : public PlayFabBaseModel
        {
            std::list<Variable> Variables;
            std::list<std::string> Variants;

            TreatmentAssignment() :
                PlayFabBaseModel(),
                Variables(),
                Variants()
            {}

            TreatmentAssignment(const TreatmentAssignment& src) :
                PlayFabBaseModel(),
                Variables(src.Variables),
                Variants(src.Variants)
            {}

            ~TreatmentAssignment() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilO(input["Variables"], Variables);
                FromJsonUtilS(input["Variants"], Variants);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_Variables; ToJsonUtilO(Variables, each_Variables); output["Variables"] = each_Variables;
                Json::Value each_Variants; ToJsonUtilS(Variants, each_Variants); output["Variants"] = each_Variants;
                return output;
            }
        };

        struct LoginResult : public PlayFabLoginResultCommon
        {
            Boxed<EntityTokenResponse> EntityToken;
            Boxed<GetPlayerCombinedInfoResultPayload> InfoResultPayload;
            Boxed<time_t> LastLoginTime;
            bool NewlyCreated;
            std::string PlayFabId;
            std::string SessionTicket;
            Boxed<UserSettings> SettingsForUser;
            Boxed<TreatmentAssignment> pfTreatmentAssignment;

            LoginResult() :
                PlayFabLoginResultCommon(),
                EntityToken(),
                InfoResultPayload(),
                LastLoginTime(),
                NewlyCreated(),
                PlayFabId(),
                SessionTicket(),
                SettingsForUser(),
                pfTreatmentAssignment()
            {}

            LoginResult(const LoginResult& src) :
                PlayFabLoginResultCommon(),
                EntityToken(src.EntityToken),
                InfoResultPayload(src.InfoResultPayload),
                LastLoginTime(src.LastLoginTime),
                NewlyCreated(src.NewlyCreated),
                PlayFabId(src.PlayFabId),
                SessionTicket(src.SessionTicket),
                SettingsForUser(src.SettingsForUser),
                pfTreatmentAssignment(src.pfTreatmentAssignment)
            {}

            ~LoginResult() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilO(input["EntityToken"], EntityToken);
                FromJsonUtilO(input["InfoResultPayload"], InfoResultPayload);
                FromJsonUtilT(input["LastLoginTime"], LastLoginTime);
                FromJsonUtilP(input["NewlyCreated"], NewlyCreated);
                FromJsonUtilS(input["PlayFabId"], PlayFabId);
                FromJsonUtilS(input["SessionTicket"], SessionTicket);
                FromJsonUtilO(input["SettingsForUser"], SettingsForUser);
                FromJsonUtilO(input["TreatmentAssignment"], pfTreatmentAssignment);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_EntityToken; ToJsonUtilO(EntityToken, each_EntityToken); output["EntityToken"] = each_EntityToken;
                Json::Value each_InfoResultPayload; ToJsonUtilO(InfoResultPayload, each_InfoResultPayload); output["InfoResultPayload"] = each_InfoResultPayload;
                Json::Value each_LastLoginTime; ToJsonUtilT(LastLoginTime, each_LastLoginTime); output["LastLoginTime"] = each_LastLoginTime;
                Json::Value each_NewlyCreated; ToJsonUtilP(NewlyCreated, each_NewlyCreated); output["NewlyCreated"] = each_NewlyCreated;
                Json::Value each_PlayFabId; ToJsonUtilS(PlayFabId, each_PlayFabId); output["PlayFabId"] = each_PlayFabId;
                Json::Value each_SessionTicket; ToJsonUtilS(SessionTicket, each_SessionTicket); output["SessionTicket"] = each_SessionTicket;
                Json::Value each_SettingsForUser; ToJsonUtilO(SettingsForUser, each_SettingsForUser); output["SettingsForUser"] = each_SettingsForUser;
                Json::Value each_pfTreatmentAssignment; ToJsonUtilO(pfTreatmentAssignment, each_pfTreatmentAssignment); output["TreatmentAssignment"] = each_pfTreatmentAssignment;
                return output;
            }
        };

        struct LoginWithAndroidDeviceIDRequest : public PlayFabRequestCommon
        {
            std::string AndroidDevice;
            std::string AndroidDeviceId;
            Boxed<bool> CreateAccount;
            std::map<std::string, std::string> CustomTags;
            std::string EncryptedRequest;
            Boxed<GetPlayerCombinedInfoRequestParams> InfoRequestParameters;
            std::string OS;
            std::string PlayerSecret;
            std::string TitleId;

            LoginWithAndroidDeviceIDRequest() :
                PlayFabRequestCommon(),
                AndroidDevice(),
                AndroidDeviceId(),
                CreateAccount(),
                CustomTags(),
                EncryptedRequest(),
                InfoRequestParameters(),
                OS(),
                PlayerSecret(),
                TitleId()
            {}

            LoginWithAndroidDeviceIDRequest(const LoginWithAndroidDeviceIDRequest& src) :
                PlayFabRequestCommon(),
                AndroidDevice(src.AndroidDevice),
                AndroidDeviceId(src.AndroidDeviceId),
                CreateAccount(src.CreateAccount),
                CustomTags(src.CustomTags),
                EncryptedRequest(src.EncryptedRequest),
                InfoRequestParameters(src.InfoRequestParameters),
                OS(src.OS),
                PlayerSecret(src.PlayerSecret),
                TitleId(src.TitleId)
            {}

            ~LoginWithAndroidDeviceIDRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["AndroidDevice"], AndroidDevice);
                FromJsonUtilS(input["AndroidDeviceId"], AndroidDeviceId);
                FromJsonUtilP(input["CreateAccount"], CreateAccount);
                FromJsonUtilS(input["CustomTags"], CustomTags);
                FromJsonUtilS(input["EncryptedRequest"], EncryptedRequest);
                FromJsonUtilO(input["InfoRequestParameters"], InfoRequestParameters);
                FromJsonUtilS(input["OS"], OS);
                FromJsonUtilS(input["PlayerSecret"], PlayerSecret);
                FromJsonUtilS(input["TitleId"], TitleId);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_AndroidDevice; ToJsonUtilS(AndroidDevice, each_AndroidDevice); output["AndroidDevice"] = each_AndroidDevice;
                Json::Value each_AndroidDeviceId; ToJsonUtilS(AndroidDeviceId, each_AndroidDeviceId); output["AndroidDeviceId"] = each_AndroidDeviceId;
                Json::Value each_CreateAccount; ToJsonUtilP(CreateAccount, each_CreateAccount); output["CreateAccount"] = each_CreateAccount;
                Json::Value each_CustomTags; ToJsonUtilS(CustomTags, each_CustomTags); output["CustomTags"] = each_CustomTags;
                Json::Value each_EncryptedRequest; ToJsonUtilS(EncryptedRequest, each_EncryptedRequest); output["EncryptedRequest"] = each_EncryptedRequest;
                Json::Value each_InfoRequestParameters; ToJsonUtilO(InfoRequestParameters, each_InfoRequestParameters); output["InfoRequestParameters"] = each_InfoRequestParameters;
                Json::Value each_OS; ToJsonUtilS(OS, each_OS); output["OS"] = each_OS;
                Json::Value each_PlayerSecret; ToJsonUtilS(PlayerSecret, each_PlayerSecret); output["PlayerSecret"] = each_PlayerSecret;
                Json::Value each_TitleId; ToJsonUtilS(TitleId, each_TitleId); output["TitleId"] = each_TitleId;
                return output;
            }
        };

        struct LoginWithAppleRequest : public PlayFabRequestCommon
        {
            Boxed<bool> CreateAccount;
            std::map<std::string, std::string> CustomTags;
            std::string EncryptedRequest;
            std::string IdentityToken;
            Boxed<GetPlayerCombinedInfoRequestParams> InfoRequestParameters;
            std::string PlayerSecret;
            std::string TitleId;

            LoginWithAppleRequest() :
                PlayFabRequestCommon(),
                CreateAccount(),
                CustomTags(),
                EncryptedRequest(),
                IdentityToken(),
                InfoRequestParameters(),
                PlayerSecret(),
                TitleId()
            {}

            LoginWithAppleRequest(const LoginWithAppleRequest& src) :
                PlayFabRequestCommon(),
                CreateAccount(src.CreateAccount),
                CustomTags(src.CustomTags),
                EncryptedRequest(src.EncryptedRequest),
                IdentityToken(src.IdentityToken),
                InfoRequestParameters(src.InfoRequestParameters),
                PlayerSecret(src.PlayerSecret),
                TitleId(src.TitleId)
            {}

            ~LoginWithAppleRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilP(input["CreateAccount"], CreateAccount);
                FromJsonUtilS(input["CustomTags"], CustomTags);
                FromJsonUtilS(input["EncryptedRequest"], EncryptedRequest);
                FromJsonUtilS(input["IdentityToken"], IdentityToken);
                FromJsonUtilO(input["InfoRequestParameters"], InfoRequestParameters);
                FromJsonUtilS(input["PlayerSecret"], PlayerSecret);
                FromJsonUtilS(input["TitleId"], TitleId);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_CreateAccount; ToJsonUtilP(CreateAccount, each_CreateAccount); output["CreateAccount"] = each_CreateAccount;
                Json::Value each_CustomTags; ToJsonUtilS(CustomTags, each_CustomTags); output["CustomTags"] = each_CustomTags;
                Json::Value each_EncryptedRequest; ToJsonUtilS(EncryptedRequest, each_EncryptedRequest); output["EncryptedRequest"] = each_EncryptedRequest;
                Json::Value each_IdentityToken; ToJsonUtilS(IdentityToken, each_IdentityToken); output["IdentityToken"] = each_IdentityToken;
                Json::Value each_InfoRequestParameters; ToJsonUtilO(InfoRequestParameters, each_InfoRequestParameters); output["InfoRequestParameters"] = each_InfoRequestParameters;
                Json::Value each_PlayerSecret; ToJsonUtilS(PlayerSecret, each_PlayerSecret); output["PlayerSecret"] = each_PlayerSecret;
                Json::Value each_TitleId; ToJsonUtilS(TitleId, each_TitleId); output["TitleId"] = each_TitleId;
                return output;
            }
        };

        struct LoginWithCustomIDRequest : public PlayFabRequestCommon
        {
            Boxed<bool> CreateAccount;
            std::string CustomId;
            std::map<std::string, std::string> CustomTags;
            std::string EncryptedRequest;
            Boxed<GetPlayerCombinedInfoRequestParams> InfoRequestParameters;
            std::string PlayerSecret;
            std::string TitleId;

            LoginWithCustomIDRequest() :
                PlayFabRequestCommon(),
                CreateAccount(),
                CustomId(),
                CustomTags(),
                EncryptedRequest(),
                InfoRequestParameters(),
                PlayerSecret(),
                TitleId()
            {}

            LoginWithCustomIDRequest(const LoginWithCustomIDRequest& src) :
                PlayFabRequestCommon(),
                CreateAccount(src.CreateAccount),
                CustomId(src.CustomId),
                CustomTags(src.CustomTags),
                EncryptedRequest(src.EncryptedRequest),
                InfoRequestParameters(src.InfoRequestParameters),
                PlayerSecret(src.PlayerSecret),
                TitleId(src.TitleId)
            {}

            ~LoginWithCustomIDRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilP(input["CreateAccount"], CreateAccount);
                FromJsonUtilS(input["CustomId"], CustomId);
                FromJsonUtilS(input["CustomTags"], CustomTags);
                FromJsonUtilS(input["EncryptedRequest"], EncryptedRequest);
                FromJsonUtilO(input["InfoRequestParameters"], InfoRequestParameters);
                FromJsonUtilS(input["PlayerSecret"], PlayerSecret);
                FromJsonUtilS(input["TitleId"], TitleId);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_CreateAccount; ToJsonUtilP(CreateAccount, each_CreateAccount); output["CreateAccount"] = each_CreateAccount;
                Json::Value each_CustomId; ToJsonUtilS(CustomId, each_CustomId); output["CustomId"] = each_CustomId;
                Json::Value each_CustomTags; ToJsonUtilS(CustomTags, each_CustomTags); output["CustomTags"] = each_CustomTags;
                Json::Value each_EncryptedRequest; ToJsonUtilS(EncryptedRequest, each_EncryptedRequest); output["EncryptedRequest"] = each_EncryptedRequest;
                Json::Value each_InfoRequestParameters; ToJsonUtilO(InfoRequestParameters, each_InfoRequestParameters); output["InfoRequestParameters"] = each_InfoRequestParameters;
                Json::Value each_PlayerSecret; ToJsonUtilS(PlayerSecret, each_PlayerSecret); output["PlayerSecret"] = each_PlayerSecret;
                Json::Value each_TitleId; ToJsonUtilS(TitleId, each_TitleId); output["TitleId"] = each_TitleId;
                return output;
            }
        };

        struct LoginWithEmailAddressRequest : public PlayFabRequestCommon
        {
            std::map<std::string, std::string> CustomTags;
            std::string Email;
            Boxed<GetPlayerCombinedInfoRequestParams> InfoRequestParameters;
            std::string Password;
            std::string TitleId;

            LoginWithEmailAddressRequest() :
                PlayFabRequestCommon(),
                CustomTags(),
                Email(),
                InfoRequestParameters(),
                Password(),
                TitleId()
            {}

            LoginWithEmailAddressRequest(const LoginWithEmailAddressRequest& src) :
                PlayFabRequestCommon(),
                CustomTags(src.CustomTags),
                Email(src.Email),
                InfoRequestParameters(src.InfoRequestParameters),
                Password(src.Password),
                TitleId(src.TitleId)
            {}

            ~LoginWithEmailAddressRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["CustomTags"], CustomTags);
                FromJsonUtilS(input["Email"], Email);
                FromJsonUtilO(input["InfoRequestParameters"], InfoRequestParameters);
                FromJsonUtilS(input["Password"], Password);
                FromJsonUtilS(input["TitleId"], TitleId);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_CustomTags; ToJsonUtilS(CustomTags, each_CustomTags); output["CustomTags"] = each_CustomTags;
                Json::Value each_Email; ToJsonUtilS(Email, each_Email); output["Email"] = each_Email;
                Json::Value each_InfoRequestParameters; ToJsonUtilO(InfoRequestParameters, each_InfoRequestParameters); output["InfoRequestParameters"] = each_InfoRequestParameters;
                Json::Value each_Password; ToJsonUtilS(Password, each_Password); output["Password"] = each_Password;
                Json::Value each_TitleId; ToJsonUtilS(TitleId, each_TitleId); output["TitleId"] = each_TitleId;
                return output;
            }
        };

        struct LoginWithFacebookInstantGamesIdRequest : public PlayFabRequestCommon
        {
            Boxed<bool> CreateAccount;
            std::map<std::string, std::string> CustomTags;
            std::string EncryptedRequest;
            std::string FacebookInstantGamesSignature;
            Boxed<GetPlayerCombinedInfoRequestParams> InfoRequestParameters;
            std::string PlayerSecret;
            std::string TitleId;

            LoginWithFacebookInstantGamesIdRequest() :
                PlayFabRequestCommon(),
                CreateAccount(),
                CustomTags(),
                EncryptedRequest(),
                FacebookInstantGamesSignature(),
                InfoRequestParameters(),
                PlayerSecret(),
                TitleId()
            {}

            LoginWithFacebookInstantGamesIdRequest(const LoginWithFacebookInstantGamesIdRequest& src) :
                PlayFabRequestCommon(),
                CreateAccount(src.CreateAccount),
                CustomTags(src.CustomTags),
                EncryptedRequest(src.EncryptedRequest),
                FacebookInstantGamesSignature(src.FacebookInstantGamesSignature),
                InfoRequestParameters(src.InfoRequestParameters),
                PlayerSecret(src.PlayerSecret),
                TitleId(src.TitleId)
            {}

            ~LoginWithFacebookInstantGamesIdRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilP(input["CreateAccount"], CreateAccount);
                FromJsonUtilS(input["CustomTags"], CustomTags);
                FromJsonUtilS(input["EncryptedRequest"], EncryptedRequest);
                FromJsonUtilS(input["FacebookInstantGamesSignature"], FacebookInstantGamesSignature);
                FromJsonUtilO(input["InfoRequestParameters"], InfoRequestParameters);
                FromJsonUtilS(input["PlayerSecret"], PlayerSecret);
                FromJsonUtilS(input["TitleId"], TitleId);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_CreateAccount; ToJsonUtilP(CreateAccount, each_CreateAccount); output["CreateAccount"] = each_CreateAccount;
                Json::Value each_CustomTags; ToJsonUtilS(CustomTags, each_CustomTags); output["CustomTags"] = each_CustomTags;
                Json::Value each_EncryptedRequest; ToJsonUtilS(EncryptedRequest, each_EncryptedRequest); output["EncryptedRequest"] = each_EncryptedRequest;
                Json::Value each_FacebookInstantGamesSignature; ToJsonUtilS(FacebookInstantGamesSignature, each_FacebookInstantGamesSignature); output["FacebookInstantGamesSignature"] = each_FacebookInstantGamesSignature;
                Json::Value each_InfoRequestParameters; ToJsonUtilO(InfoRequestParameters, each_InfoRequestParameters); output["InfoRequestParameters"] = each_InfoRequestParameters;
                Json::Value each_PlayerSecret; ToJsonUtilS(PlayerSecret, each_PlayerSecret); output["PlayerSecret"] = each_PlayerSecret;
                Json::Value each_TitleId; ToJsonUtilS(TitleId, each_TitleId); output["TitleId"] = each_TitleId;
                return output;
            }
        };

        struct LoginWithFacebookRequest : public PlayFabRequestCommon
        {
            std::string AccessToken;
            Boxed<bool> CreateAccount;
            std::map<std::string, std::string> CustomTags;
            std::string EncryptedRequest;
            Boxed<GetPlayerCombinedInfoRequestParams> InfoRequestParameters;
            std::string PlayerSecret;
            std::string TitleId;

            LoginWithFacebookRequest() :
                PlayFabRequestCommon(),
                AccessToken(),
                CreateAccount(),
                CustomTags(),
                EncryptedRequest(),
                InfoRequestParameters(),
                PlayerSecret(),
                TitleId()
            {}

            LoginWithFacebookRequest(const LoginWithFacebookRequest& src) :
                PlayFabRequestCommon(),
                AccessToken(src.AccessToken),
                CreateAccount(src.CreateAccount),
                CustomTags(src.CustomTags),
                EncryptedRequest(src.EncryptedRequest),
                InfoRequestParameters(src.InfoRequestParameters),
                PlayerSecret(src.PlayerSecret),
                TitleId(src.TitleId)
            {}

            ~LoginWithFacebookRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["AccessToken"], AccessToken);
                FromJsonUtilP(input["CreateAccount"], CreateAccount);
                FromJsonUtilS(input["CustomTags"], CustomTags);
                FromJsonUtilS(input["EncryptedRequest"], EncryptedRequest);
                FromJsonUtilO(input["InfoRequestParameters"], InfoRequestParameters);
                FromJsonUtilS(input["PlayerSecret"], PlayerSecret);
                FromJsonUtilS(input["TitleId"], TitleId);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_AccessToken; ToJsonUtilS(AccessToken, each_AccessToken); output["AccessToken"] = each_AccessToken;
                Json::Value each_CreateAccount; ToJsonUtilP(CreateAccount, each_CreateAccount); output["CreateAccount"] = each_CreateAccount;
                Json::Value each_CustomTags; ToJsonUtilS(CustomTags, each_CustomTags); output["CustomTags"] = each_CustomTags;
                Json::Value each_EncryptedRequest; ToJsonUtilS(EncryptedRequest, each_EncryptedRequest); output["EncryptedRequest"] = each_EncryptedRequest;
                Json::Value each_InfoRequestParameters; ToJsonUtilO(InfoRequestParameters, each_InfoRequestParameters); output["InfoRequestParameters"] = each_InfoRequestParameters;
                Json::Value each_PlayerSecret; ToJsonUtilS(PlayerSecret, each_PlayerSecret); output["PlayerSecret"] = each_PlayerSecret;
                Json::Value each_TitleId; ToJsonUtilS(TitleId, each_TitleId); output["TitleId"] = each_TitleId;
                return output;
            }
        };

        struct LoginWithGameCenterRequest : public PlayFabRequestCommon
        {
            Boxed<bool> CreateAccount;
            std::map<std::string, std::string> CustomTags;
            std::string EncryptedRequest;
            Boxed<GetPlayerCombinedInfoRequestParams> InfoRequestParameters;
            std::string PlayerId;
            std::string PlayerSecret;
            std::string PublicKeyUrl;
            std::string Salt;
            std::string Signature;
            std::string Timestamp;
            std::string TitleId;

            LoginWithGameCenterRequest() :
                PlayFabRequestCommon(),
                CreateAccount(),
                CustomTags(),
                EncryptedRequest(),
                InfoRequestParameters(),
                PlayerId(),
                PlayerSecret(),
                PublicKeyUrl(),
                Salt(),
                Signature(),
                Timestamp(),
                TitleId()
            {}

            LoginWithGameCenterRequest(const LoginWithGameCenterRequest& src) :
                PlayFabRequestCommon(),
                CreateAccount(src.CreateAccount),
                CustomTags(src.CustomTags),
                EncryptedRequest(src.EncryptedRequest),
                InfoRequestParameters(src.InfoRequestParameters),
                PlayerId(src.PlayerId),
                PlayerSecret(src.PlayerSecret),
                PublicKeyUrl(src.PublicKeyUrl),
                Salt(src.Salt),
                Signature(src.Signature),
                Timestamp(src.Timestamp),
                TitleId(src.TitleId)
            {}

            ~LoginWithGameCenterRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilP(input["CreateAccount"], CreateAccount);
                FromJsonUtilS(input["CustomTags"], CustomTags);
                FromJsonUtilS(input["EncryptedRequest"], EncryptedRequest);
                FromJsonUtilO(input["InfoRequestParameters"], InfoRequestParameters);
                FromJsonUtilS(input["PlayerId"], PlayerId);
                FromJsonUtilS(input["PlayerSecret"], PlayerSecret);
                FromJsonUtilS(input["PublicKeyUrl"], PublicKeyUrl);
                FromJsonUtilS(input["Salt"], Salt);
                FromJsonUtilS(input["Signature"], Signature);
                FromJsonUtilS(input["Timestamp"], Timestamp);
                FromJsonUtilS(input["TitleId"], TitleId);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_CreateAccount; ToJsonUtilP(CreateAccount, each_CreateAccount); output["CreateAccount"] = each_CreateAccount;
                Json::Value each_CustomTags; ToJsonUtilS(CustomTags, each_CustomTags); output["CustomTags"] = each_CustomTags;
                Json::Value each_EncryptedRequest; ToJsonUtilS(EncryptedRequest, each_EncryptedRequest); output["EncryptedRequest"] = each_EncryptedRequest;
                Json::Value each_InfoRequestParameters; ToJsonUtilO(InfoRequestParameters, each_InfoRequestParameters); output["InfoRequestParameters"] = each_InfoRequestParameters;
                Json::Value each_PlayerId; ToJsonUtilS(PlayerId, each_PlayerId); output["PlayerId"] = each_PlayerId;
                Json::Value each_PlayerSecret; ToJsonUtilS(PlayerSecret, each_PlayerSecret); output["PlayerSecret"] = each_PlayerSecret;
                Json::Value each_PublicKeyUrl; ToJsonUtilS(PublicKeyUrl, each_PublicKeyUrl); output["PublicKeyUrl"] = each_PublicKeyUrl;
                Json::Value each_Salt; ToJsonUtilS(Salt, each_Salt); output["Salt"] = each_Salt;
                Json::Value each_Signature; ToJsonUtilS(Signature, each_Signature); output["Signature"] = each_Signature;
                Json::Value each_Timestamp; ToJsonUtilS(Timestamp, each_Timestamp); output["Timestamp"] = each_Timestamp;
                Json::Value each_TitleId; ToJsonUtilS(TitleId, each_TitleId); output["TitleId"] = each_TitleId;
                return output;
            }
        };

        struct LoginWithGoogleAccountRequest : public PlayFabRequestCommon
        {
            Boxed<bool> CreateAccount;
            std::map<std::string, std::string> CustomTags;
            std::string EncryptedRequest;
            Boxed<GetPlayerCombinedInfoRequestParams> InfoRequestParameters;
            std::string PlayerSecret;
            std::string ServerAuthCode;
            std::string TitleId;

            LoginWithGoogleAccountRequest() :
                PlayFabRequestCommon(),
                CreateAccount(),
                CustomTags(),
                EncryptedRequest(),
                InfoRequestParameters(),
                PlayerSecret(),
                ServerAuthCode(),
                TitleId()
            {}

            LoginWithGoogleAccountRequest(const LoginWithGoogleAccountRequest& src) :
                PlayFabRequestCommon(),
                CreateAccount(src.CreateAccount),
                CustomTags(src.CustomTags),
                EncryptedRequest(src.EncryptedRequest),
                InfoRequestParameters(src.InfoRequestParameters),
                PlayerSecret(src.PlayerSecret),
                ServerAuthCode(src.ServerAuthCode),
                TitleId(src.TitleId)
            {}

            ~LoginWithGoogleAccountRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilP(input["CreateAccount"], CreateAccount);
                FromJsonUtilS(input["CustomTags"], CustomTags);
                FromJsonUtilS(input["EncryptedRequest"], EncryptedRequest);
                FromJsonUtilO(input["InfoRequestParameters"], InfoRequestParameters);
                FromJsonUtilS(input["PlayerSecret"], PlayerSecret);
                FromJsonUtilS(input["ServerAuthCode"], ServerAuthCode);
                FromJsonUtilS(input["TitleId"], TitleId);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_CreateAccount; ToJsonUtilP(CreateAccount, each_CreateAccount); output["CreateAccount"] = each_CreateAccount;
                Json::Value each_CustomTags; ToJsonUtilS(CustomTags, each_CustomTags); output["CustomTags"] = each_CustomTags;
                Json::Value each_EncryptedRequest; ToJsonUtilS(EncryptedRequest, each_EncryptedRequest); output["EncryptedRequest"] = each_EncryptedRequest;
                Json::Value each_InfoRequestParameters; ToJsonUtilO(InfoRequestParameters, each_InfoRequestParameters); output["InfoRequestParameters"] = each_InfoRequestParameters;
                Json::Value each_PlayerSecret; ToJsonUtilS(PlayerSecret, each_PlayerSecret); output["PlayerSecret"] = each_PlayerSecret;
                Json::Value each_ServerAuthCode; ToJsonUtilS(ServerAuthCode, each_ServerAuthCode); output["ServerAuthCode"] = each_ServerAuthCode;
                Json::Value each_TitleId; ToJsonUtilS(TitleId, each_TitleId); output["TitleId"] = each_TitleId;
                return output;
            }
        };

        struct LoginWithIOSDeviceIDRequest : public PlayFabRequestCommon
        {
            Boxed<bool> CreateAccount;
            std::map<std::string, std::string> CustomTags;
            std::string DeviceId;
            std::string DeviceModel;
            std::string EncryptedRequest;
            Boxed<GetPlayerCombinedInfoRequestParams> InfoRequestParameters;
            std::string OS;
            std::string PlayerSecret;
            std::string TitleId;

            LoginWithIOSDeviceIDRequest() :
                PlayFabRequestCommon(),
                CreateAccount(),
                CustomTags(),
                DeviceId(),
                DeviceModel(),
                EncryptedRequest(),
                InfoRequestParameters(),
                OS(),
                PlayerSecret(),
                TitleId()
            {}

            LoginWithIOSDeviceIDRequest(const LoginWithIOSDeviceIDRequest& src) :
                PlayFabRequestCommon(),
                CreateAccount(src.CreateAccount),
                CustomTags(src.CustomTags),
                DeviceId(src.DeviceId),
                DeviceModel(src.DeviceModel),
                EncryptedRequest(src.EncryptedRequest),
                InfoRequestParameters(src.InfoRequestParameters),
                OS(src.OS),
                PlayerSecret(src.PlayerSecret),
                TitleId(src.TitleId)
            {}

            ~LoginWithIOSDeviceIDRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilP(input["CreateAccount"], CreateAccount);
                FromJsonUtilS(input["CustomTags"], CustomTags);
                FromJsonUtilS(input["DeviceId"], DeviceId);
                FromJsonUtilS(input["DeviceModel"], DeviceModel);
                FromJsonUtilS(input["EncryptedRequest"], EncryptedRequest);
                FromJsonUtilO(input["InfoRequestParameters"], InfoRequestParameters);
                FromJsonUtilS(input["OS"], OS);
                FromJsonUtilS(input["PlayerSecret"], PlayerSecret);
                FromJsonUtilS(input["TitleId"], TitleId);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_CreateAccount; ToJsonUtilP(CreateAccount, each_CreateAccount); output["CreateAccount"] = each_CreateAccount;
                Json::Value each_CustomTags; ToJsonUtilS(CustomTags, each_CustomTags); output["CustomTags"] = each_CustomTags;
                Json::Value each_DeviceId; ToJsonUtilS(DeviceId, each_DeviceId); output["DeviceId"] = each_DeviceId;
                Json::Value each_DeviceModel; ToJsonUtilS(DeviceModel, each_DeviceModel); output["DeviceModel"] = each_DeviceModel;
                Json::Value each_EncryptedRequest; ToJsonUtilS(EncryptedRequest, each_EncryptedRequest); output["EncryptedRequest"] = each_EncryptedRequest;
                Json::Value each_InfoRequestParameters; ToJsonUtilO(InfoRequestParameters, each_InfoRequestParameters); output["InfoRequestParameters"] = each_InfoRequestParameters;
                Json::Value each_OS; ToJsonUtilS(OS, each_OS); output["OS"] = each_OS;
                Json::Value each_PlayerSecret; ToJsonUtilS(PlayerSecret, each_PlayerSecret); output["PlayerSecret"] = each_PlayerSecret;
                Json::Value each_TitleId; ToJsonUtilS(TitleId, each_TitleId); output["TitleId"] = each_TitleId;
                return output;
            }
        };

        struct LoginWithKongregateRequest : public PlayFabRequestCommon
        {
            std::string AuthTicket;
            Boxed<bool> CreateAccount;
            std::map<std::string, std::string> CustomTags;
            std::string EncryptedRequest;
            Boxed<GetPlayerCombinedInfoRequestParams> InfoRequestParameters;
            std::string KongregateId;
            std::string PlayerSecret;
            std::string TitleId;

            LoginWithKongregateRequest() :
                PlayFabRequestCommon(),
                AuthTicket(),
                CreateAccount(),
                CustomTags(),
                EncryptedRequest(),
                InfoRequestParameters(),
                KongregateId(),
                PlayerSecret(),
                TitleId()
            {}

            LoginWithKongregateRequest(const LoginWithKongregateRequest& src) :
                PlayFabRequestCommon(),
                AuthTicket(src.AuthTicket),
                CreateAccount(src.CreateAccount),
                CustomTags(src.CustomTags),
                EncryptedRequest(src.EncryptedRequest),
                InfoRequestParameters(src.InfoRequestParameters),
                KongregateId(src.KongregateId),
                PlayerSecret(src.PlayerSecret),
                TitleId(src.TitleId)
            {}

            ~LoginWithKongregateRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["AuthTicket"], AuthTicket);
                FromJsonUtilP(input["CreateAccount"], CreateAccount);
                FromJsonUtilS(input["CustomTags"], CustomTags);
                FromJsonUtilS(input["EncryptedRequest"], EncryptedRequest);
                FromJsonUtilO(input["InfoRequestParameters"], InfoRequestParameters);
                FromJsonUtilS(input["KongregateId"], KongregateId);
                FromJsonUtilS(input["PlayerSecret"], PlayerSecret);
                FromJsonUtilS(input["TitleId"], TitleId);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_AuthTicket; ToJsonUtilS(AuthTicket, each_AuthTicket); output["AuthTicket"] = each_AuthTicket;
                Json::Value each_CreateAccount; ToJsonUtilP(CreateAccount, each_CreateAccount); output["CreateAccount"] = each_CreateAccount;
                Json::Value each_CustomTags; ToJsonUtilS(CustomTags, each_CustomTags); output["CustomTags"] = each_CustomTags;
                Json::Value each_EncryptedRequest; ToJsonUtilS(EncryptedRequest, each_EncryptedRequest); output["EncryptedRequest"] = each_EncryptedRequest;
                Json::Value each_InfoRequestParameters; ToJsonUtilO(InfoRequestParameters, each_InfoRequestParameters); output["InfoRequestParameters"] = each_InfoRequestParameters;
                Json::Value each_KongregateId; ToJsonUtilS(KongregateId, each_KongregateId); output["KongregateId"] = each_KongregateId;
                Json::Value each_PlayerSecret; ToJsonUtilS(PlayerSecret, each_PlayerSecret); output["PlayerSecret"] = each_PlayerSecret;
                Json::Value each_TitleId; ToJsonUtilS(TitleId, each_TitleId); output["TitleId"] = each_TitleId;
                return output;
            }
        };

        struct LoginWithNintendoServiceAccountRequest : public PlayFabRequestCommon
        {
            Boxed<bool> CreateAccount;
            std::map<std::string, std::string> CustomTags;
            std::string EncryptedRequest;
            std::string IdentityToken;
            Boxed<GetPlayerCombinedInfoRequestParams> InfoRequestParameters;
            std::string PlayerSecret;
            std::string TitleId;

            LoginWithNintendoServiceAccountRequest() :
                PlayFabRequestCommon(),
                CreateAccount(),
                CustomTags(),
                EncryptedRequest(),
                IdentityToken(),
                InfoRequestParameters(),
                PlayerSecret(),
                TitleId()
            {}

            LoginWithNintendoServiceAccountRequest(const LoginWithNintendoServiceAccountRequest& src) :
                PlayFabRequestCommon(),
                CreateAccount(src.CreateAccount),
                CustomTags(src.CustomTags),
                EncryptedRequest(src.EncryptedRequest),
                IdentityToken(src.IdentityToken),
                InfoRequestParameters(src.InfoRequestParameters),
                PlayerSecret(src.PlayerSecret),
                TitleId(src.TitleId)
            {}

            ~LoginWithNintendoServiceAccountRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilP(input["CreateAccount"], CreateAccount);
                FromJsonUtilS(input["CustomTags"], CustomTags);
                FromJsonUtilS(input["EncryptedRequest"], EncryptedRequest);
                FromJsonUtilS(input["IdentityToken"], IdentityToken);
                FromJsonUtilO(input["InfoRequestParameters"], InfoRequestParameters);
                FromJsonUtilS(input["PlayerSecret"], PlayerSecret);
                FromJsonUtilS(input["TitleId"], TitleId);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_CreateAccount; ToJsonUtilP(CreateAccount, each_CreateAccount); output["CreateAccount"] = each_CreateAccount;
                Json::Value each_CustomTags; ToJsonUtilS(CustomTags, each_CustomTags); output["CustomTags"] = each_CustomTags;
                Json::Value each_EncryptedRequest; ToJsonUtilS(EncryptedRequest, each_EncryptedRequest); output["EncryptedRequest"] = each_EncryptedRequest;
                Json::Value each_IdentityToken; ToJsonUtilS(IdentityToken, each_IdentityToken); output["IdentityToken"] = each_IdentityToken;
                Json::Value each_InfoRequestParameters; ToJsonUtilO(InfoRequestParameters, each_InfoRequestParameters); output["InfoRequestParameters"] = each_InfoRequestParameters;
                Json::Value each_PlayerSecret; ToJsonUtilS(PlayerSecret, each_PlayerSecret); output["PlayerSecret"] = each_PlayerSecret;
                Json::Value each_TitleId; ToJsonUtilS(TitleId, each_TitleId); output["TitleId"] = each_TitleId;
                return output;
            }
        };

        struct LoginWithNintendoSwitchDeviceIdRequest : public PlayFabRequestCommon
        {
            Boxed<bool> CreateAccount;
            std::map<std::string, std::string> CustomTags;
            std::string EncryptedRequest;
            Boxed<GetPlayerCombinedInfoRequestParams> InfoRequestParameters;
            std::string NintendoSwitchDeviceId;
            std::string PlayerSecret;
            std::string TitleId;

            LoginWithNintendoSwitchDeviceIdRequest() :
                PlayFabRequestCommon(),
                CreateAccount(),
                CustomTags(),
                EncryptedRequest(),
                InfoRequestParameters(),
                NintendoSwitchDeviceId(),
                PlayerSecret(),
                TitleId()
            {}

            LoginWithNintendoSwitchDeviceIdRequest(const LoginWithNintendoSwitchDeviceIdRequest& src) :
                PlayFabRequestCommon(),
                CreateAccount(src.CreateAccount),
                CustomTags(src.CustomTags),
                EncryptedRequest(src.EncryptedRequest),
                InfoRequestParameters(src.InfoRequestParameters),
                NintendoSwitchDeviceId(src.NintendoSwitchDeviceId),
                PlayerSecret(src.PlayerSecret),
                TitleId(src.TitleId)
            {}

            ~LoginWithNintendoSwitchDeviceIdRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilP(input["CreateAccount"], CreateAccount);
                FromJsonUtilS(input["CustomTags"], CustomTags);
                FromJsonUtilS(input["EncryptedRequest"], EncryptedRequest);
                FromJsonUtilO(input["InfoRequestParameters"], InfoRequestParameters);
                FromJsonUtilS(input["NintendoSwitchDeviceId"], NintendoSwitchDeviceId);
                FromJsonUtilS(input["PlayerSecret"], PlayerSecret);
                FromJsonUtilS(input["TitleId"], TitleId);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_CreateAccount; ToJsonUtilP(CreateAccount, each_CreateAccount); output["CreateAccount"] = each_CreateAccount;
                Json::Value each_CustomTags; ToJsonUtilS(CustomTags, each_CustomTags); output["CustomTags"] = each_CustomTags;
                Json::Value each_EncryptedRequest; ToJsonUtilS(EncryptedRequest, each_EncryptedRequest); output["EncryptedRequest"] = each_EncryptedRequest;
                Json::Value each_InfoRequestParameters; ToJsonUtilO(InfoRequestParameters, each_InfoRequestParameters); output["InfoRequestParameters"] = each_InfoRequestParameters;
                Json::Value each_NintendoSwitchDeviceId; ToJsonUtilS(NintendoSwitchDeviceId, each_NintendoSwitchDeviceId); output["NintendoSwitchDeviceId"] = each_NintendoSwitchDeviceId;
                Json::Value each_PlayerSecret; ToJsonUtilS(PlayerSecret, each_PlayerSecret); output["PlayerSecret"] = each_PlayerSecret;
                Json::Value each_TitleId; ToJsonUtilS(TitleId, each_TitleId); output["TitleId"] = each_TitleId;
                return output;
            }
        };

        struct LoginWithOpenIdConnectRequest : public PlayFabRequestCommon
        {
            std::string ConnectionId;
            Boxed<bool> CreateAccount;
            std::map<std::string, std::string> CustomTags;
            std::string EncryptedRequest;
            std::string IdToken;
            Boxed<GetPlayerCombinedInfoRequestParams> InfoRequestParameters;
            std::string PlayerSecret;
            std::string TitleId;

            LoginWithOpenIdConnectRequest() :
                PlayFabRequestCommon(),
                ConnectionId(),
                CreateAccount(),
                CustomTags(),
                EncryptedRequest(),
                IdToken(),
                InfoRequestParameters(),
                PlayerSecret(),
                TitleId()
            {}

            LoginWithOpenIdConnectRequest(const LoginWithOpenIdConnectRequest& src) :
                PlayFabRequestCommon(),
                ConnectionId(src.ConnectionId),
                CreateAccount(src.CreateAccount),
                CustomTags(src.CustomTags),
                EncryptedRequest(src.EncryptedRequest),
                IdToken(src.IdToken),
                InfoRequestParameters(src.InfoRequestParameters),
                PlayerSecret(src.PlayerSecret),
                TitleId(src.TitleId)
            {}

            ~LoginWithOpenIdConnectRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["ConnectionId"], ConnectionId);
                FromJsonUtilP(input["CreateAccount"], CreateAccount);
                FromJsonUtilS(input["CustomTags"], CustomTags);
                FromJsonUtilS(input["EncryptedRequest"], EncryptedRequest);
                FromJsonUtilS(input["IdToken"], IdToken);
                FromJsonUtilO(input["InfoRequestParameters"], InfoRequestParameters);
                FromJsonUtilS(input["PlayerSecret"], PlayerSecret);
                FromJsonUtilS(input["TitleId"], TitleId);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_ConnectionId; ToJsonUtilS(ConnectionId, each_ConnectionId); output["ConnectionId"] = each_ConnectionId;
                Json::Value each_CreateAccount; ToJsonUtilP(CreateAccount, each_CreateAccount); output["CreateAccount"] = each_CreateAccount;
                Json::Value each_CustomTags; ToJsonUtilS(CustomTags, each_CustomTags); output["CustomTags"] = each_CustomTags;
                Json::Value each_EncryptedRequest; ToJsonUtilS(EncryptedRequest, each_EncryptedRequest); output["EncryptedRequest"] = each_EncryptedRequest;
                Json::Value each_IdToken; ToJsonUtilS(IdToken, each_IdToken); output["IdToken"] = each_IdToken;
                Json::Value each_InfoRequestParameters; ToJsonUtilO(InfoRequestParameters, each_InfoRequestParameters); output["InfoRequestParameters"] = each_InfoRequestParameters;
                Json::Value each_PlayerSecret; ToJsonUtilS(PlayerSecret, each_PlayerSecret); output["PlayerSecret"] = each_PlayerSecret;
                Json::Value each_TitleId; ToJsonUtilS(TitleId, each_TitleId); output["TitleId"] = each_TitleId;
                return output;
            }
        };

        struct LoginWithPlayFabRequest : public PlayFabRequestCommon
        {
            std::map<std::string, std::string> CustomTags;
            Boxed<GetPlayerCombinedInfoRequestParams> InfoRequestParameters;
            std::string Password;
            std::string TitleId;
            std::string Username;

            LoginWithPlayFabRequest() :
                PlayFabRequestCommon(),
                CustomTags(),
                InfoRequestParameters(),
                Password(),
                TitleId(),
                Username()
            {}

            LoginWithPlayFabRequest(const LoginWithPlayFabRequest& src) :
                PlayFabRequestCommon(),
                CustomTags(src.CustomTags),
                InfoRequestParameters(src.InfoRequestParameters),
                Password(src.Password),
                TitleId(src.TitleId),
                Username(src.Username)
            {}

            ~LoginWithPlayFabRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["CustomTags"], CustomTags);
                FromJsonUtilO(input["InfoRequestParameters"], InfoRequestParameters);
                FromJsonUtilS(input["Password"], Password);
                FromJsonUtilS(input["TitleId"], TitleId);
                FromJsonUtilS(input["Username"], Username);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_CustomTags; ToJsonUtilS(CustomTags, each_CustomTags); output["CustomTags"] = each_CustomTags;
                Json::Value each_InfoRequestParameters; ToJsonUtilO(InfoRequestParameters, each_InfoRequestParameters); output["InfoRequestParameters"] = each_InfoRequestParameters;
                Json::Value each_Password; ToJsonUtilS(Password, each_Password); output["Password"] = each_Password;
                Json::Value each_TitleId; ToJsonUtilS(TitleId, each_TitleId); output["TitleId"] = each_TitleId;
                Json::Value each_Username; ToJsonUtilS(Username, each_Username); output["Username"] = each_Username;
                return output;
            }
        };

        struct LoginWithPSNRequest : public PlayFabRequestCommon
        {
            std::string AuthCode;
            Boxed<bool> CreateAccount;
            std::map<std::string, std::string> CustomTags;
            std::string EncryptedRequest;
            Boxed<GetPlayerCombinedInfoRequestParams> InfoRequestParameters;
            Boxed<Int32> IssuerId;
            std::string PlayerSecret;
            std::string RedirectUri;
            std::string TitleId;

            LoginWithPSNRequest() :
                PlayFabRequestCommon(),
                AuthCode(),
                CreateAccount(),
                CustomTags(),
                EncryptedRequest(),
                InfoRequestParameters(),
                IssuerId(),
                PlayerSecret(),
                RedirectUri(),
                TitleId()
            {}

            LoginWithPSNRequest(const LoginWithPSNRequest& src) :
                PlayFabRequestCommon(),
                AuthCode(src.AuthCode),
                CreateAccount(src.CreateAccount),
                CustomTags(src.CustomTags),
                EncryptedRequest(src.EncryptedRequest),
                InfoRequestParameters(src.InfoRequestParameters),
                IssuerId(src.IssuerId),
                PlayerSecret(src.PlayerSecret),
                RedirectUri(src.RedirectUri),
                TitleId(src.TitleId)
            {}

            ~LoginWithPSNRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["AuthCode"], AuthCode);
                FromJsonUtilP(input["CreateAccount"], CreateAccount);
                FromJsonUtilS(input["CustomTags"], CustomTags);
                FromJsonUtilS(input["EncryptedRequest"], EncryptedRequest);
                FromJsonUtilO(input["InfoRequestParameters"], InfoRequestParameters);
                FromJsonUtilP(input["IssuerId"], IssuerId);
                FromJsonUtilS(input["PlayerSecret"], PlayerSecret);
                FromJsonUtilS(input["RedirectUri"], RedirectUri);
                FromJsonUtilS(input["TitleId"], TitleId);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_AuthCode; ToJsonUtilS(AuthCode, each_AuthCode); output["AuthCode"] = each_AuthCode;
                Json::Value each_CreateAccount; ToJsonUtilP(CreateAccount, each_CreateAccount); output["CreateAccount"] = each_CreateAccount;
                Json::Value each_CustomTags; ToJsonUtilS(CustomTags, each_CustomTags); output["CustomTags"] = each_CustomTags;
                Json::Value each_EncryptedRequest; ToJsonUtilS(EncryptedRequest, each_EncryptedRequest); output["EncryptedRequest"] = each_EncryptedRequest;
                Json::Value each_InfoRequestParameters; ToJsonUtilO(InfoRequestParameters, each_InfoRequestParameters); output["InfoRequestParameters"] = each_InfoRequestParameters;
                Json::Value each_IssuerId; ToJsonUtilP(IssuerId, each_IssuerId); output["IssuerId"] = each_IssuerId;
                Json::Value each_PlayerSecret; ToJsonUtilS(PlayerSecret, each_PlayerSecret); output["PlayerSecret"] = each_PlayerSecret;
                Json::Value each_RedirectUri; ToJsonUtilS(RedirectUri, each_RedirectUri); output["RedirectUri"] = each_RedirectUri;
                Json::Value each_TitleId; ToJsonUtilS(TitleId, each_TitleId); output["TitleId"] = each_TitleId;
                return output;
            }
        };

        struct LoginWithSteamRequest : public PlayFabRequestCommon
        {
            Boxed<bool> CreateAccount;
            std::map<std::string, std::string> CustomTags;
            std::string EncryptedRequest;
            Boxed<GetPlayerCombinedInfoRequestParams> InfoRequestParameters;
            std::string PlayerSecret;
            std::string SteamTicket;
            std::string TitleId;

            LoginWithSteamRequest() :
                PlayFabRequestCommon(),
                CreateAccount(),
                CustomTags(),
                EncryptedRequest(),
                InfoRequestParameters(),
                PlayerSecret(),
                SteamTicket(),
                TitleId()
            {}

            LoginWithSteamRequest(const LoginWithSteamRequest& src) :
                PlayFabRequestCommon(),
                CreateAccount(src.CreateAccount),
                CustomTags(src.CustomTags),
                EncryptedRequest(src.EncryptedRequest),
                InfoRequestParameters(src.InfoRequestParameters),
                PlayerSecret(src.PlayerSecret),
                SteamTicket(src.SteamTicket),
                TitleId(src.TitleId)
            {}

            ~LoginWithSteamRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilP(input["CreateAccount"], CreateAccount);
                FromJsonUtilS(input["CustomTags"], CustomTags);
                FromJsonUtilS(input["EncryptedRequest"], EncryptedRequest);
                FromJsonUtilO(input["InfoRequestParameters"], InfoRequestParameters);
                FromJsonUtilS(input["PlayerSecret"], PlayerSecret);
                FromJsonUtilS(input["SteamTicket"], SteamTicket);
                FromJsonUtilS(input["TitleId"], TitleId);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_CreateAccount; ToJsonUtilP(CreateAccount, each_CreateAccount); output["CreateAccount"] = each_CreateAccount;
                Json::Value each_CustomTags; ToJsonUtilS(CustomTags, each_CustomTags); output["CustomTags"] = each_CustomTags;
                Json::Value each_EncryptedRequest; ToJsonUtilS(EncryptedRequest, each_EncryptedRequest); output["EncryptedRequest"] = each_EncryptedRequest;
                Json::Value each_InfoRequestParameters; ToJsonUtilO(InfoRequestParameters, each_InfoRequestParameters); output["InfoRequestParameters"] = each_InfoRequestParameters;
                Json::Value each_PlayerSecret; ToJsonUtilS(PlayerSecret, each_PlayerSecret); output["PlayerSecret"] = each_PlayerSecret;
                Json::Value each_SteamTicket; ToJsonUtilS(SteamTicket, each_SteamTicket); output["SteamTicket"] = each_SteamTicket;
                Json::Value each_TitleId; ToJsonUtilS(TitleId, each_TitleId); output["TitleId"] = each_TitleId;
                return output;
            }
        };

        struct LoginWithTwitchRequest : public PlayFabRequestCommon
        {
            std::string AccessToken;
            Boxed<bool> CreateAccount;
            std::map<std::string, std::string> CustomTags;
            std::string EncryptedRequest;
            Boxed<GetPlayerCombinedInfoRequestParams> InfoRequestParameters;
            std::string PlayerSecret;
            std::string TitleId;

            LoginWithTwitchRequest() :
                PlayFabRequestCommon(),
                AccessToken(),
                CreateAccount(),
                CustomTags(),
                EncryptedRequest(),
                InfoRequestParameters(),
                PlayerSecret(),
                TitleId()
            {}

            LoginWithTwitchRequest(const LoginWithTwitchRequest& src) :
                PlayFabRequestCommon(),
                AccessToken(src.AccessToken),
                CreateAccount(src.CreateAccount),
                CustomTags(src.CustomTags),
                EncryptedRequest(src.EncryptedRequest),
                InfoRequestParameters(src.InfoRequestParameters),
                PlayerSecret(src.PlayerSecret),
                TitleId(src.TitleId)
            {}

            ~LoginWithTwitchRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["AccessToken"], AccessToken);
                FromJsonUtilP(input["CreateAccount"], CreateAccount);
                FromJsonUtilS(input["CustomTags"], CustomTags);
                FromJsonUtilS(input["EncryptedRequest"], EncryptedRequest);
                FromJsonUtilO(input["InfoRequestParameters"], InfoRequestParameters);
                FromJsonUtilS(input["PlayerSecret"], PlayerSecret);
                FromJsonUtilS(input["TitleId"], TitleId);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_AccessToken; ToJsonUtilS(AccessToken, each_AccessToken); output["AccessToken"] = each_AccessToken;
                Json::Value each_CreateAccount; ToJsonUtilP(CreateAccount, each_CreateAccount); output["CreateAccount"] = each_CreateAccount;
                Json::Value each_CustomTags; ToJsonUtilS(CustomTags, each_CustomTags); output["CustomTags"] = each_CustomTags;
                Json::Value each_EncryptedRequest; ToJsonUtilS(EncryptedRequest, each_EncryptedRequest); output["EncryptedRequest"] = each_EncryptedRequest;
                Json::Value each_InfoRequestParameters; ToJsonUtilO(InfoRequestParameters, each_InfoRequestParameters); output["InfoRequestParameters"] = each_InfoRequestParameters;
                Json::Value each_PlayerSecret; ToJsonUtilS(PlayerSecret, each_PlayerSecret); output["PlayerSecret"] = each_PlayerSecret;
                Json::Value each_TitleId; ToJsonUtilS(TitleId, each_TitleId); output["TitleId"] = each_TitleId;
                return output;
            }
        };

        struct LoginWithXboxRequest : public PlayFabRequestCommon
        {
            Boxed<bool> CreateAccount;
            std::map<std::string, std::string> CustomTags;
            std::string EncryptedRequest;
            Boxed<GetPlayerCombinedInfoRequestParams> InfoRequestParameters;
            std::string PlayerSecret;
            std::string TitleId;
            std::string XboxToken;

            LoginWithXboxRequest() :
                PlayFabRequestCommon(),
                CreateAccount(),
                CustomTags(),
                EncryptedRequest(),
                InfoRequestParameters(),
                PlayerSecret(),
                TitleId(),
                XboxToken()
            {}

            LoginWithXboxRequest(const LoginWithXboxRequest& src) :
                PlayFabRequestCommon(),
                CreateAccount(src.CreateAccount),
                CustomTags(src.CustomTags),
                EncryptedRequest(src.EncryptedRequest),
                InfoRequestParameters(src.InfoRequestParameters),
                PlayerSecret(src.PlayerSecret),
                TitleId(src.TitleId),
                XboxToken(src.XboxToken)
            {}

            ~LoginWithXboxRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilP(input["CreateAccount"], CreateAccount);
                FromJsonUtilS(input["CustomTags"], CustomTags);
                FromJsonUtilS(input["EncryptedRequest"], EncryptedRequest);
                FromJsonUtilO(input["InfoRequestParameters"], InfoRequestParameters);
                FromJsonUtilS(input["PlayerSecret"], PlayerSecret);
                FromJsonUtilS(input["TitleId"], TitleId);
                FromJsonUtilS(input["XboxToken"], XboxToken);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_CreateAccount; ToJsonUtilP(CreateAccount, each_CreateAccount); output["CreateAccount"] = each_CreateAccount;
                Json::Value each_CustomTags; ToJsonUtilS(CustomTags, each_CustomTags); output["CustomTags"] = each_CustomTags;
                Json::Value each_EncryptedRequest; ToJsonUtilS(EncryptedRequest, each_EncryptedRequest); output["EncryptedRequest"] = each_EncryptedRequest;
                Json::Value each_InfoRequestParameters; ToJsonUtilO(InfoRequestParameters, each_InfoRequestParameters); output["InfoRequestParameters"] = each_InfoRequestParameters;
                Json::Value each_PlayerSecret; ToJsonUtilS(PlayerSecret, each_PlayerSecret); output["PlayerSecret"] = each_PlayerSecret;
                Json::Value each_TitleId; ToJsonUtilS(TitleId, each_TitleId); output["TitleId"] = each_TitleId;
                Json::Value each_XboxToken; ToJsonUtilS(XboxToken, each_XboxToken); output["XboxToken"] = each_XboxToken;
                return output;
            }
        };

        struct MatchmakeRequest : public PlayFabRequestCommon
        {
            std::string BuildVersion;
            std::string CharacterId;
            std::map<std::string, std::string> CustomTags;
            std::string GameMode;
            std::string LobbyId;
            Boxed<Region> pfRegion;
            Boxed<bool> StartNewIfNoneFound;
            std::string StatisticName;
            Boxed<CollectionFilter> TagFilter;

            MatchmakeRequest() :
                PlayFabRequestCommon(),
                BuildVersion(),
                CharacterId(),
                CustomTags(),
                GameMode(),
                LobbyId(),
                pfRegion(),
                StartNewIfNoneFound(),
                StatisticName(),
                TagFilter()
            {}

            MatchmakeRequest(const MatchmakeRequest& src) :
                PlayFabRequestCommon(),
                BuildVersion(src.BuildVersion),
                CharacterId(src.CharacterId),
                CustomTags(src.CustomTags),
                GameMode(src.GameMode),
                LobbyId(src.LobbyId),
                pfRegion(src.pfRegion),
                StartNewIfNoneFound(src.StartNewIfNoneFound),
                StatisticName(src.StatisticName),
                TagFilter(src.TagFilter)
            {}

            ~MatchmakeRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["BuildVersion"], BuildVersion);
                FromJsonUtilS(input["CharacterId"], CharacterId);
                FromJsonUtilS(input["CustomTags"], CustomTags);
                FromJsonUtilS(input["GameMode"], GameMode);
                FromJsonUtilS(input["LobbyId"], LobbyId);
                FromJsonUtilE(input["Region"], pfRegion);
                FromJsonUtilP(input["StartNewIfNoneFound"], StartNewIfNoneFound);
                FromJsonUtilS(input["StatisticName"], StatisticName);
                FromJsonUtilO(input["TagFilter"], TagFilter);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_BuildVersion; ToJsonUtilS(BuildVersion, each_BuildVersion); output["BuildVersion"] = each_BuildVersion;
                Json::Value each_CharacterId; ToJsonUtilS(CharacterId, each_CharacterId); output["CharacterId"] = each_CharacterId;
                Json::Value each_CustomTags; ToJsonUtilS(CustomTags, each_CustomTags); output["CustomTags"] = each_CustomTags;
                Json::Value each_GameMode; ToJsonUtilS(GameMode, each_GameMode); output["GameMode"] = each_GameMode;
                Json::Value each_LobbyId; ToJsonUtilS(LobbyId, each_LobbyId); output["LobbyId"] = each_LobbyId;
                Json::Value each_pfRegion; ToJsonUtilE(pfRegion, each_pfRegion); output["Region"] = each_pfRegion;
                Json::Value each_StartNewIfNoneFound; ToJsonUtilP(StartNewIfNoneFound, each_StartNewIfNoneFound); output["StartNewIfNoneFound"] = each_StartNewIfNoneFound;
                Json::Value each_StatisticName; ToJsonUtilS(StatisticName, each_StatisticName); output["StatisticName"] = each_StatisticName;
                Json::Value each_TagFilter; ToJsonUtilO(TagFilter, each_TagFilter); output["TagFilter"] = each_TagFilter;
                return output;
            }
        };

        struct MatchmakeResult : public PlayFabResultCommon
        {
            std::string Expires;
            std::string LobbyID;
            Boxed<Int32> PollWaitTimeMS;
            std::string ServerIPV4Address;
            std::string ServerIPV6Address;
            Boxed<Int32> ServerPort;
            std::string ServerPublicDNSName;
            Boxed<MatchmakeStatus> Status;
            std::string Ticket;

            MatchmakeResult() :
                PlayFabResultCommon(),
                Expires(),
                LobbyID(),
                PollWaitTimeMS(),
                ServerIPV4Address(),
                ServerIPV6Address(),
                ServerPort(),
                ServerPublicDNSName(),
                Status(),
                Ticket()
            {}

            MatchmakeResult(const MatchmakeResult& src) :
                PlayFabResultCommon(),
                Expires(src.Expires),
                LobbyID(src.LobbyID),
                PollWaitTimeMS(src.PollWaitTimeMS),
                ServerIPV4Address(src.ServerIPV4Address),
                ServerIPV6Address(src.ServerIPV6Address),
                ServerPort(src.ServerPort),
                ServerPublicDNSName(src.ServerPublicDNSName),
                Status(src.Status),
                Ticket(src.Ticket)
            {}

            ~MatchmakeResult() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["Expires"], Expires);
                FromJsonUtilS(input["LobbyID"], LobbyID);
                FromJsonUtilP(input["PollWaitTimeMS"], PollWaitTimeMS);
                FromJsonUtilS(input["ServerIPV4Address"], ServerIPV4Address);
                FromJsonUtilS(input["ServerIPV6Address"], ServerIPV6Address);
                FromJsonUtilP(input["ServerPort"], ServerPort);
                FromJsonUtilS(input["ServerPublicDNSName"], ServerPublicDNSName);
                FromJsonUtilE(input["Status"], Status);
                FromJsonUtilS(input["Ticket"], Ticket);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_Expires; ToJsonUtilS(Expires, each_Expires); output["Expires"] = each_Expires;
                Json::Value each_LobbyID; ToJsonUtilS(LobbyID, each_LobbyID); output["LobbyID"] = each_LobbyID;
                Json::Value each_PollWaitTimeMS; ToJsonUtilP(PollWaitTimeMS, each_PollWaitTimeMS); output["PollWaitTimeMS"] = each_PollWaitTimeMS;
                Json::Value each_ServerIPV4Address; ToJsonUtilS(ServerIPV4Address, each_ServerIPV4Address); output["ServerIPV4Address"] = each_ServerIPV4Address;
                Json::Value each_ServerIPV6Address; ToJsonUtilS(ServerIPV6Address, each_ServerIPV6Address); output["ServerIPV6Address"] = each_ServerIPV6Address;
                Json::Value each_ServerPort; ToJsonUtilP(ServerPort, each_ServerPort); output["ServerPort"] = each_ServerPort;
                Json::Value each_ServerPublicDNSName; ToJsonUtilS(ServerPublicDNSName, each_ServerPublicDNSName); output["ServerPublicDNSName"] = each_ServerPublicDNSName;
                Json::Value each_Status; ToJsonUtilE(Status, each_Status); output["Status"] = each_Status;
                Json::Value each_Ticket; ToJsonUtilS(Ticket, each_Ticket); output["Ticket"] = each_Ticket;
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

        struct OpenTradeRequest : public PlayFabRequestCommon
        {
            std::list<std::string> AllowedPlayerIds;
            std::list<std::string> OfferedInventoryInstanceIds;
            std::list<std::string> RequestedCatalogItemIds;

            OpenTradeRequest() :
                PlayFabRequestCommon(),
                AllowedPlayerIds(),
                OfferedInventoryInstanceIds(),
                RequestedCatalogItemIds()
            {}

            OpenTradeRequest(const OpenTradeRequest& src) :
                PlayFabRequestCommon(),
                AllowedPlayerIds(src.AllowedPlayerIds),
                OfferedInventoryInstanceIds(src.OfferedInventoryInstanceIds),
                RequestedCatalogItemIds(src.RequestedCatalogItemIds)
            {}

            ~OpenTradeRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["AllowedPlayerIds"], AllowedPlayerIds);
                FromJsonUtilS(input["OfferedInventoryInstanceIds"], OfferedInventoryInstanceIds);
                FromJsonUtilS(input["RequestedCatalogItemIds"], RequestedCatalogItemIds);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_AllowedPlayerIds; ToJsonUtilS(AllowedPlayerIds, each_AllowedPlayerIds); output["AllowedPlayerIds"] = each_AllowedPlayerIds;
                Json::Value each_OfferedInventoryInstanceIds; ToJsonUtilS(OfferedInventoryInstanceIds, each_OfferedInventoryInstanceIds); output["OfferedInventoryInstanceIds"] = each_OfferedInventoryInstanceIds;
                Json::Value each_RequestedCatalogItemIds; ToJsonUtilS(RequestedCatalogItemIds, each_RequestedCatalogItemIds); output["RequestedCatalogItemIds"] = each_RequestedCatalogItemIds;
                return output;
            }
        };

        struct OpenTradeResponse : public PlayFabResultCommon
        {
            Boxed<TradeInfo> Trade;

            OpenTradeResponse() :
                PlayFabResultCommon(),
                Trade()
            {}

            OpenTradeResponse(const OpenTradeResponse& src) :
                PlayFabResultCommon(),
                Trade(src.Trade)
            {}

            ~OpenTradeResponse() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilO(input["Trade"], Trade);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_Trade; ToJsonUtilO(Trade, each_Trade); output["Trade"] = each_Trade;
                return output;
            }
        };

        struct PayForPurchaseRequest : public PlayFabRequestCommon
        {
            std::string Currency;
            std::map<std::string, std::string> CustomTags;
            std::string OrderId;
            std::string ProviderName;
            std::string ProviderTransactionId;

            PayForPurchaseRequest() :
                PlayFabRequestCommon(),
                Currency(),
                CustomTags(),
                OrderId(),
                ProviderName(),
                ProviderTransactionId()
            {}

            PayForPurchaseRequest(const PayForPurchaseRequest& src) :
                PlayFabRequestCommon(),
                Currency(src.Currency),
                CustomTags(src.CustomTags),
                OrderId(src.OrderId),
                ProviderName(src.ProviderName),
                ProviderTransactionId(src.ProviderTransactionId)
            {}

            ~PayForPurchaseRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["Currency"], Currency);
                FromJsonUtilS(input["CustomTags"], CustomTags);
                FromJsonUtilS(input["OrderId"], OrderId);
                FromJsonUtilS(input["ProviderName"], ProviderName);
                FromJsonUtilS(input["ProviderTransactionId"], ProviderTransactionId);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_Currency; ToJsonUtilS(Currency, each_Currency); output["Currency"] = each_Currency;
                Json::Value each_CustomTags; ToJsonUtilS(CustomTags, each_CustomTags); output["CustomTags"] = each_CustomTags;
                Json::Value each_OrderId; ToJsonUtilS(OrderId, each_OrderId); output["OrderId"] = each_OrderId;
                Json::Value each_ProviderName; ToJsonUtilS(ProviderName, each_ProviderName); output["ProviderName"] = each_ProviderName;
                Json::Value each_ProviderTransactionId; ToJsonUtilS(ProviderTransactionId, each_ProviderTransactionId); output["ProviderTransactionId"] = each_ProviderTransactionId;
                return output;
            }
        };

        struct PayForPurchaseResult : public PlayFabResultCommon
        {
            Uint32 CreditApplied;
            std::string OrderId;
            std::string ProviderData;
            std::string ProviderToken;
            std::string PurchaseConfirmationPageURL;
            std::string PurchaseCurrency;
            Uint32 PurchasePrice;
            Boxed<TransactionStatus> Status;
            std::map<std::string, Int32> VCAmount;
            std::map<std::string, Int32> VirtualCurrency;

            PayForPurchaseResult() :
                PlayFabResultCommon(),
                CreditApplied(),
                OrderId(),
                ProviderData(),
                ProviderToken(),
                PurchaseConfirmationPageURL(),
                PurchaseCurrency(),
                PurchasePrice(),
                Status(),
                VCAmount(),
                VirtualCurrency()
            {}

            PayForPurchaseResult(const PayForPurchaseResult& src) :
                PlayFabResultCommon(),
                CreditApplied(src.CreditApplied),
                OrderId(src.OrderId),
                ProviderData(src.ProviderData),
                ProviderToken(src.ProviderToken),
                PurchaseConfirmationPageURL(src.PurchaseConfirmationPageURL),
                PurchaseCurrency(src.PurchaseCurrency),
                PurchasePrice(src.PurchasePrice),
                Status(src.Status),
                VCAmount(src.VCAmount),
                VirtualCurrency(src.VirtualCurrency)
            {}

            ~PayForPurchaseResult() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilP(input["CreditApplied"], CreditApplied);
                FromJsonUtilS(input["OrderId"], OrderId);
                FromJsonUtilS(input["ProviderData"], ProviderData);
                FromJsonUtilS(input["ProviderToken"], ProviderToken);
                FromJsonUtilS(input["PurchaseConfirmationPageURL"], PurchaseConfirmationPageURL);
                FromJsonUtilS(input["PurchaseCurrency"], PurchaseCurrency);
                FromJsonUtilP(input["PurchasePrice"], PurchasePrice);
                FromJsonUtilE(input["Status"], Status);
                FromJsonUtilP(input["VCAmount"], VCAmount);
                FromJsonUtilP(input["VirtualCurrency"], VirtualCurrency);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_CreditApplied; ToJsonUtilP(CreditApplied, each_CreditApplied); output["CreditApplied"] = each_CreditApplied;
                Json::Value each_OrderId; ToJsonUtilS(OrderId, each_OrderId); output["OrderId"] = each_OrderId;
                Json::Value each_ProviderData; ToJsonUtilS(ProviderData, each_ProviderData); output["ProviderData"] = each_ProviderData;
                Json::Value each_ProviderToken; ToJsonUtilS(ProviderToken, each_ProviderToken); output["ProviderToken"] = each_ProviderToken;
                Json::Value each_PurchaseConfirmationPageURL; ToJsonUtilS(PurchaseConfirmationPageURL, each_PurchaseConfirmationPageURL); output["PurchaseConfirmationPageURL"] = each_PurchaseConfirmationPageURL;
                Json::Value each_PurchaseCurrency; ToJsonUtilS(PurchaseCurrency, each_PurchaseCurrency); output["PurchaseCurrency"] = each_PurchaseCurrency;
                Json::Value each_PurchasePrice; ToJsonUtilP(PurchasePrice, each_PurchasePrice); output["PurchasePrice"] = each_PurchasePrice;
                Json::Value each_Status; ToJsonUtilE(Status, each_Status); output["Status"] = each_Status;
                Json::Value each_VCAmount; ToJsonUtilP(VCAmount, each_VCAmount); output["VCAmount"] = each_VCAmount;
                Json::Value each_VirtualCurrency; ToJsonUtilP(VirtualCurrency, each_VirtualCurrency); output["VirtualCurrency"] = each_VirtualCurrency;
                return output;
            }
        };

        struct PaymentOption : public PlayFabBaseModel
        {
            std::string Currency;
            Uint32 Price;
            std::string ProviderName;
            Uint32 StoreCredit;

            PaymentOption() :
                PlayFabBaseModel(),
                Currency(),
                Price(),
                ProviderName(),
                StoreCredit()
            {}

            PaymentOption(const PaymentOption& src) :
                PlayFabBaseModel(),
                Currency(src.Currency),
                Price(src.Price),
                ProviderName(src.ProviderName),
                StoreCredit(src.StoreCredit)
            {}

            ~PaymentOption() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["Currency"], Currency);
                FromJsonUtilP(input["Price"], Price);
                FromJsonUtilS(input["ProviderName"], ProviderName);
                FromJsonUtilP(input["StoreCredit"], StoreCredit);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_Currency; ToJsonUtilS(Currency, each_Currency); output["Currency"] = each_Currency;
                Json::Value each_Price; ToJsonUtilP(Price, each_Price); output["Price"] = each_Price;
                Json::Value each_ProviderName; ToJsonUtilS(ProviderName, each_ProviderName); output["ProviderName"] = each_ProviderName;
                Json::Value each_StoreCredit; ToJsonUtilP(StoreCredit, each_StoreCredit); output["StoreCredit"] = each_StoreCredit;
                return output;
            }
        };

        struct PurchaseItemRequest : public PlayFabRequestCommon
        {
            std::string CatalogVersion;
            std::string CharacterId;
            std::map<std::string, std::string> CustomTags;
            std::string ItemId;
            Int32 Price;
            std::string StoreId;
            std::string VirtualCurrency;

            PurchaseItemRequest() :
                PlayFabRequestCommon(),
                CatalogVersion(),
                CharacterId(),
                CustomTags(),
                ItemId(),
                Price(),
                StoreId(),
                VirtualCurrency()
            {}

            PurchaseItemRequest(const PurchaseItemRequest& src) :
                PlayFabRequestCommon(),
                CatalogVersion(src.CatalogVersion),
                CharacterId(src.CharacterId),
                CustomTags(src.CustomTags),
                ItemId(src.ItemId),
                Price(src.Price),
                StoreId(src.StoreId),
                VirtualCurrency(src.VirtualCurrency)
            {}

            ~PurchaseItemRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["CatalogVersion"], CatalogVersion);
                FromJsonUtilS(input["CharacterId"], CharacterId);
                FromJsonUtilS(input["CustomTags"], CustomTags);
                FromJsonUtilS(input["ItemId"], ItemId);
                FromJsonUtilP(input["Price"], Price);
                FromJsonUtilS(input["StoreId"], StoreId);
                FromJsonUtilS(input["VirtualCurrency"], VirtualCurrency);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_CatalogVersion; ToJsonUtilS(CatalogVersion, each_CatalogVersion); output["CatalogVersion"] = each_CatalogVersion;
                Json::Value each_CharacterId; ToJsonUtilS(CharacterId, each_CharacterId); output["CharacterId"] = each_CharacterId;
                Json::Value each_CustomTags; ToJsonUtilS(CustomTags, each_CustomTags); output["CustomTags"] = each_CustomTags;
                Json::Value each_ItemId; ToJsonUtilS(ItemId, each_ItemId); output["ItemId"] = each_ItemId;
                Json::Value each_Price; ToJsonUtilP(Price, each_Price); output["Price"] = each_Price;
                Json::Value each_StoreId; ToJsonUtilS(StoreId, each_StoreId); output["StoreId"] = each_StoreId;
                Json::Value each_VirtualCurrency; ToJsonUtilS(VirtualCurrency, each_VirtualCurrency); output["VirtualCurrency"] = each_VirtualCurrency;
                return output;
            }
        };

        struct PurchaseItemResult : public PlayFabResultCommon
        {
            std::list<ItemInstance> Items;

            PurchaseItemResult() :
                PlayFabResultCommon(),
                Items()
            {}

            PurchaseItemResult(const PurchaseItemResult& src) :
                PlayFabResultCommon(),
                Items(src.Items)
            {}

            ~PurchaseItemResult() = default;

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

        struct PurchaseReceiptFulfillment : public PlayFabBaseModel
        {
            std::list<ItemInstance> FulfilledItems;
            std::string RecordedPriceSource;
            std::string RecordedTransactionCurrency;
            Boxed<Uint32> RecordedTransactionTotal;

            PurchaseReceiptFulfillment() :
                PlayFabBaseModel(),
                FulfilledItems(),
                RecordedPriceSource(),
                RecordedTransactionCurrency(),
                RecordedTransactionTotal()
            {}

            PurchaseReceiptFulfillment(const PurchaseReceiptFulfillment& src) :
                PlayFabBaseModel(),
                FulfilledItems(src.FulfilledItems),
                RecordedPriceSource(src.RecordedPriceSource),
                RecordedTransactionCurrency(src.RecordedTransactionCurrency),
                RecordedTransactionTotal(src.RecordedTransactionTotal)
            {}

            ~PurchaseReceiptFulfillment() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilO(input["FulfilledItems"], FulfilledItems);
                FromJsonUtilS(input["RecordedPriceSource"], RecordedPriceSource);
                FromJsonUtilS(input["RecordedTransactionCurrency"], RecordedTransactionCurrency);
                FromJsonUtilP(input["RecordedTransactionTotal"], RecordedTransactionTotal);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_FulfilledItems; ToJsonUtilO(FulfilledItems, each_FulfilledItems); output["FulfilledItems"] = each_FulfilledItems;
                Json::Value each_RecordedPriceSource; ToJsonUtilS(RecordedPriceSource, each_RecordedPriceSource); output["RecordedPriceSource"] = each_RecordedPriceSource;
                Json::Value each_RecordedTransactionCurrency; ToJsonUtilS(RecordedTransactionCurrency, each_RecordedTransactionCurrency); output["RecordedTransactionCurrency"] = each_RecordedTransactionCurrency;
                Json::Value each_RecordedTransactionTotal; ToJsonUtilP(RecordedTransactionTotal, each_RecordedTransactionTotal); output["RecordedTransactionTotal"] = each_RecordedTransactionTotal;
                return output;
            }
        };

        struct RedeemCouponRequest : public PlayFabRequestCommon
        {
            std::string CatalogVersion;
            std::string CharacterId;
            std::string CouponCode;
            std::map<std::string, std::string> CustomTags;

            RedeemCouponRequest() :
                PlayFabRequestCommon(),
                CatalogVersion(),
                CharacterId(),
                CouponCode(),
                CustomTags()
            {}

            RedeemCouponRequest(const RedeemCouponRequest& src) :
                PlayFabRequestCommon(),
                CatalogVersion(src.CatalogVersion),
                CharacterId(src.CharacterId),
                CouponCode(src.CouponCode),
                CustomTags(src.CustomTags)
            {}

            ~RedeemCouponRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["CatalogVersion"], CatalogVersion);
                FromJsonUtilS(input["CharacterId"], CharacterId);
                FromJsonUtilS(input["CouponCode"], CouponCode);
                FromJsonUtilS(input["CustomTags"], CustomTags);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_CatalogVersion; ToJsonUtilS(CatalogVersion, each_CatalogVersion); output["CatalogVersion"] = each_CatalogVersion;
                Json::Value each_CharacterId; ToJsonUtilS(CharacterId, each_CharacterId); output["CharacterId"] = each_CharacterId;
                Json::Value each_CouponCode; ToJsonUtilS(CouponCode, each_CouponCode); output["CouponCode"] = each_CouponCode;
                Json::Value each_CustomTags; ToJsonUtilS(CustomTags, each_CustomTags); output["CustomTags"] = each_CustomTags;
                return output;
            }
        };

        struct RedeemCouponResult : public PlayFabResultCommon
        {
            std::list<ItemInstance> GrantedItems;

            RedeemCouponResult() :
                PlayFabResultCommon(),
                GrantedItems()
            {}

            RedeemCouponResult(const RedeemCouponResult& src) :
                PlayFabResultCommon(),
                GrantedItems(src.GrantedItems)
            {}

            ~RedeemCouponResult() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilO(input["GrantedItems"], GrantedItems);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_GrantedItems; ToJsonUtilO(GrantedItems, each_GrantedItems); output["GrantedItems"] = each_GrantedItems;
                return output;
            }
        };

        struct RefreshPSNAuthTokenRequest : public PlayFabRequestCommon
        {
            std::string AuthCode;
            Boxed<Int32> IssuerId;
            std::string RedirectUri;

            RefreshPSNAuthTokenRequest() :
                PlayFabRequestCommon(),
                AuthCode(),
                IssuerId(),
                RedirectUri()
            {}

            RefreshPSNAuthTokenRequest(const RefreshPSNAuthTokenRequest& src) :
                PlayFabRequestCommon(),
                AuthCode(src.AuthCode),
                IssuerId(src.IssuerId),
                RedirectUri(src.RedirectUri)
            {}

            ~RefreshPSNAuthTokenRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["AuthCode"], AuthCode);
                FromJsonUtilP(input["IssuerId"], IssuerId);
                FromJsonUtilS(input["RedirectUri"], RedirectUri);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_AuthCode; ToJsonUtilS(AuthCode, each_AuthCode); output["AuthCode"] = each_AuthCode;
                Json::Value each_IssuerId; ToJsonUtilP(IssuerId, each_IssuerId); output["IssuerId"] = each_IssuerId;
                Json::Value each_RedirectUri; ToJsonUtilS(RedirectUri, each_RedirectUri); output["RedirectUri"] = each_RedirectUri;
                return output;
            }
        };

        struct RegisterForIOSPushNotificationRequest : public PlayFabRequestCommon
        {
            std::string ConfirmationMessage;
            std::string DeviceToken;
            Boxed<bool> SendPushNotificationConfirmation;

            RegisterForIOSPushNotificationRequest() :
                PlayFabRequestCommon(),
                ConfirmationMessage(),
                DeviceToken(),
                SendPushNotificationConfirmation()
            {}

            RegisterForIOSPushNotificationRequest(const RegisterForIOSPushNotificationRequest& src) :
                PlayFabRequestCommon(),
                ConfirmationMessage(src.ConfirmationMessage),
                DeviceToken(src.DeviceToken),
                SendPushNotificationConfirmation(src.SendPushNotificationConfirmation)
            {}

            ~RegisterForIOSPushNotificationRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["ConfirmationMessage"], ConfirmationMessage);
                FromJsonUtilS(input["DeviceToken"], DeviceToken);
                FromJsonUtilP(input["SendPushNotificationConfirmation"], SendPushNotificationConfirmation);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_ConfirmationMessage; ToJsonUtilS(ConfirmationMessage, each_ConfirmationMessage); output["ConfirmationMessage"] = each_ConfirmationMessage;
                Json::Value each_DeviceToken; ToJsonUtilS(DeviceToken, each_DeviceToken); output["DeviceToken"] = each_DeviceToken;
                Json::Value each_SendPushNotificationConfirmation; ToJsonUtilP(SendPushNotificationConfirmation, each_SendPushNotificationConfirmation); output["SendPushNotificationConfirmation"] = each_SendPushNotificationConfirmation;
                return output;
            }
        };

        struct RegisterForIOSPushNotificationResult : public PlayFabResultCommon
        {

            RegisterForIOSPushNotificationResult() :
                PlayFabResultCommon()
            {}

            RegisterForIOSPushNotificationResult(const RegisterForIOSPushNotificationResult&) :
                PlayFabResultCommon()
            {}

            ~RegisterForIOSPushNotificationResult() = default;

            void FromJson(const Json::Value&) override
            {
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                return output;
            }
        };

        struct RegisterPlayFabUserRequest : public PlayFabRequestCommon
        {
            std::map<std::string, std::string> CustomTags;
            std::string DisplayName;
            std::string Email;
            std::string EncryptedRequest;
            Boxed<GetPlayerCombinedInfoRequestParams> InfoRequestParameters;
            std::string Password;
            std::string PlayerSecret;
            Boxed<bool> RequireBothUsernameAndEmail;
            std::string TitleId;
            std::string Username;

            RegisterPlayFabUserRequest() :
                PlayFabRequestCommon(),
                CustomTags(),
                DisplayName(),
                Email(),
                EncryptedRequest(),
                InfoRequestParameters(),
                Password(),
                PlayerSecret(),
                RequireBothUsernameAndEmail(),
                TitleId(),
                Username()
            {}

            RegisterPlayFabUserRequest(const RegisterPlayFabUserRequest& src) :
                PlayFabRequestCommon(),
                CustomTags(src.CustomTags),
                DisplayName(src.DisplayName),
                Email(src.Email),
                EncryptedRequest(src.EncryptedRequest),
                InfoRequestParameters(src.InfoRequestParameters),
                Password(src.Password),
                PlayerSecret(src.PlayerSecret),
                RequireBothUsernameAndEmail(src.RequireBothUsernameAndEmail),
                TitleId(src.TitleId),
                Username(src.Username)
            {}

            ~RegisterPlayFabUserRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["CustomTags"], CustomTags);
                FromJsonUtilS(input["DisplayName"], DisplayName);
                FromJsonUtilS(input["Email"], Email);
                FromJsonUtilS(input["EncryptedRequest"], EncryptedRequest);
                FromJsonUtilO(input["InfoRequestParameters"], InfoRequestParameters);
                FromJsonUtilS(input["Password"], Password);
                FromJsonUtilS(input["PlayerSecret"], PlayerSecret);
                FromJsonUtilP(input["RequireBothUsernameAndEmail"], RequireBothUsernameAndEmail);
                FromJsonUtilS(input["TitleId"], TitleId);
                FromJsonUtilS(input["Username"], Username);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_CustomTags; ToJsonUtilS(CustomTags, each_CustomTags); output["CustomTags"] = each_CustomTags;
                Json::Value each_DisplayName; ToJsonUtilS(DisplayName, each_DisplayName); output["DisplayName"] = each_DisplayName;
                Json::Value each_Email; ToJsonUtilS(Email, each_Email); output["Email"] = each_Email;
                Json::Value each_EncryptedRequest; ToJsonUtilS(EncryptedRequest, each_EncryptedRequest); output["EncryptedRequest"] = each_EncryptedRequest;
                Json::Value each_InfoRequestParameters; ToJsonUtilO(InfoRequestParameters, each_InfoRequestParameters); output["InfoRequestParameters"] = each_InfoRequestParameters;
                Json::Value each_Password; ToJsonUtilS(Password, each_Password); output["Password"] = each_Password;
                Json::Value each_PlayerSecret; ToJsonUtilS(PlayerSecret, each_PlayerSecret); output["PlayerSecret"] = each_PlayerSecret;
                Json::Value each_RequireBothUsernameAndEmail; ToJsonUtilP(RequireBothUsernameAndEmail, each_RequireBothUsernameAndEmail); output["RequireBothUsernameAndEmail"] = each_RequireBothUsernameAndEmail;
                Json::Value each_TitleId; ToJsonUtilS(TitleId, each_TitleId); output["TitleId"] = each_TitleId;
                Json::Value each_Username; ToJsonUtilS(Username, each_Username); output["Username"] = each_Username;
                return output;
            }
        };

        struct RegisterPlayFabUserResult : public PlayFabResultCommon
        {
            Boxed<EntityTokenResponse> EntityToken;
            std::string PlayFabId;
            std::string SessionTicket;
            Boxed<UserSettings> SettingsForUser;
            std::string Username;

            RegisterPlayFabUserResult() :
                PlayFabResultCommon(),
                EntityToken(),
                PlayFabId(),
                SessionTicket(),
                SettingsForUser(),
                Username()
            {}

            RegisterPlayFabUserResult(const RegisterPlayFabUserResult& src) :
                PlayFabResultCommon(),
                EntityToken(src.EntityToken),
                PlayFabId(src.PlayFabId),
                SessionTicket(src.SessionTicket),
                SettingsForUser(src.SettingsForUser),
                Username(src.Username)
            {}

            ~RegisterPlayFabUserResult() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilO(input["EntityToken"], EntityToken);
                FromJsonUtilS(input["PlayFabId"], PlayFabId);
                FromJsonUtilS(input["SessionTicket"], SessionTicket);
                FromJsonUtilO(input["SettingsForUser"], SettingsForUser);
                FromJsonUtilS(input["Username"], Username);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_EntityToken; ToJsonUtilO(EntityToken, each_EntityToken); output["EntityToken"] = each_EntityToken;
                Json::Value each_PlayFabId; ToJsonUtilS(PlayFabId, each_PlayFabId); output["PlayFabId"] = each_PlayFabId;
                Json::Value each_SessionTicket; ToJsonUtilS(SessionTicket, each_SessionTicket); output["SessionTicket"] = each_SessionTicket;
                Json::Value each_SettingsForUser; ToJsonUtilO(SettingsForUser, each_SettingsForUser); output["SettingsForUser"] = each_SettingsForUser;
                Json::Value each_Username; ToJsonUtilS(Username, each_Username); output["Username"] = each_Username;
                return output;
            }
        };

        struct RemoveContactEmailRequest : public PlayFabRequestCommon
        {
            std::map<std::string, std::string> CustomTags;

            RemoveContactEmailRequest() :
                PlayFabRequestCommon(),
                CustomTags()
            {}

            RemoveContactEmailRequest(const RemoveContactEmailRequest& src) :
                PlayFabRequestCommon(),
                CustomTags(src.CustomTags)
            {}

            ~RemoveContactEmailRequest() = default;

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

        struct RemoveContactEmailResult : public PlayFabResultCommon
        {

            RemoveContactEmailResult() :
                PlayFabResultCommon()
            {}

            RemoveContactEmailResult(const RemoveContactEmailResult&) :
                PlayFabResultCommon()
            {}

            ~RemoveContactEmailResult() = default;

            void FromJson(const Json::Value&) override
            {
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                return output;
            }
        };

        struct RemoveFriendRequest : public PlayFabRequestCommon
        {
            std::string FriendPlayFabId;

            RemoveFriendRequest() :
                PlayFabRequestCommon(),
                FriendPlayFabId()
            {}

            RemoveFriendRequest(const RemoveFriendRequest& src) :
                PlayFabRequestCommon(),
                FriendPlayFabId(src.FriendPlayFabId)
            {}

            ~RemoveFriendRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["FriendPlayFabId"], FriendPlayFabId);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_FriendPlayFabId; ToJsonUtilS(FriendPlayFabId, each_FriendPlayFabId); output["FriendPlayFabId"] = each_FriendPlayFabId;
                return output;
            }
        };

        struct RemoveFriendResult : public PlayFabResultCommon
        {

            RemoveFriendResult() :
                PlayFabResultCommon()
            {}

            RemoveFriendResult(const RemoveFriendResult&) :
                PlayFabResultCommon()
            {}

            ~RemoveFriendResult() = default;

            void FromJson(const Json::Value&) override
            {
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                return output;
            }
        };

        struct RemoveGenericIDRequest : public PlayFabRequestCommon
        {
            GenericServiceId GenericId;

            RemoveGenericIDRequest() :
                PlayFabRequestCommon(),
                GenericId()
            {}

            RemoveGenericIDRequest(const RemoveGenericIDRequest& src) :
                PlayFabRequestCommon(),
                GenericId(src.GenericId)
            {}

            ~RemoveGenericIDRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilO(input["GenericId"], GenericId);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_GenericId; ToJsonUtilO(GenericId, each_GenericId); output["GenericId"] = each_GenericId;
                return output;
            }
        };

        struct RemoveGenericIDResult : public PlayFabResultCommon
        {

            RemoveGenericIDResult() :
                PlayFabResultCommon()
            {}

            RemoveGenericIDResult(const RemoveGenericIDResult&) :
                PlayFabResultCommon()
            {}

            ~RemoveGenericIDResult() = default;

            void FromJson(const Json::Value&) override
            {
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                return output;
            }
        };

        struct RemoveSharedGroupMembersRequest : public PlayFabRequestCommon
        {
            std::list<std::string> PlayFabIds;
            std::string SharedGroupId;

            RemoveSharedGroupMembersRequest() :
                PlayFabRequestCommon(),
                PlayFabIds(),
                SharedGroupId()
            {}

            RemoveSharedGroupMembersRequest(const RemoveSharedGroupMembersRequest& src) :
                PlayFabRequestCommon(),
                PlayFabIds(src.PlayFabIds),
                SharedGroupId(src.SharedGroupId)
            {}

            ~RemoveSharedGroupMembersRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["PlayFabIds"], PlayFabIds);
                FromJsonUtilS(input["SharedGroupId"], SharedGroupId);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_PlayFabIds; ToJsonUtilS(PlayFabIds, each_PlayFabIds); output["PlayFabIds"] = each_PlayFabIds;
                Json::Value each_SharedGroupId; ToJsonUtilS(SharedGroupId, each_SharedGroupId); output["SharedGroupId"] = each_SharedGroupId;
                return output;
            }
        };

        struct RemoveSharedGroupMembersResult : public PlayFabResultCommon
        {

            RemoveSharedGroupMembersResult() :
                PlayFabResultCommon()
            {}

            RemoveSharedGroupMembersResult(const RemoveSharedGroupMembersResult&) :
                PlayFabResultCommon()
            {}

            ~RemoveSharedGroupMembersResult() = default;

            void FromJson(const Json::Value&) override
            {
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                return output;
            }
        };

        struct ReportAdActivityRequest : public PlayFabRequestCommon
        {
            AdActivity Activity;
            std::map<std::string, std::string> CustomTags;
            std::string PlacementId;
            std::string RewardId;

            ReportAdActivityRequest() :
                PlayFabRequestCommon(),
                Activity(),
                CustomTags(),
                PlacementId(),
                RewardId()
            {}

            ReportAdActivityRequest(const ReportAdActivityRequest& src) :
                PlayFabRequestCommon(),
                Activity(src.Activity),
                CustomTags(src.CustomTags),
                PlacementId(src.PlacementId),
                RewardId(src.RewardId)
            {}

            ~ReportAdActivityRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonEnum(input["Activity"], Activity);
                FromJsonUtilS(input["CustomTags"], CustomTags);
                FromJsonUtilS(input["PlacementId"], PlacementId);
                FromJsonUtilS(input["RewardId"], RewardId);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_Activity; ToJsonEnum(Activity, each_Activity); output["Activity"] = each_Activity;
                Json::Value each_CustomTags; ToJsonUtilS(CustomTags, each_CustomTags); output["CustomTags"] = each_CustomTags;
                Json::Value each_PlacementId; ToJsonUtilS(PlacementId, each_PlacementId); output["PlacementId"] = each_PlacementId;
                Json::Value each_RewardId; ToJsonUtilS(RewardId, each_RewardId); output["RewardId"] = each_RewardId;
                return output;
            }
        };

        struct ReportAdActivityResult : public PlayFabResultCommon
        {

            ReportAdActivityResult() :
                PlayFabResultCommon()
            {}

            ReportAdActivityResult(const ReportAdActivityResult&) :
                PlayFabResultCommon()
            {}

            ~ReportAdActivityResult() = default;

            void FromJson(const Json::Value&) override
            {
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                return output;
            }
        };

        struct ReportPlayerClientRequest : public PlayFabRequestCommon
        {
            std::string Comment;
            std::map<std::string, std::string> CustomTags;
            std::string ReporteeId;

            ReportPlayerClientRequest() :
                PlayFabRequestCommon(),
                Comment(),
                CustomTags(),
                ReporteeId()
            {}

            ReportPlayerClientRequest(const ReportPlayerClientRequest& src) :
                PlayFabRequestCommon(),
                Comment(src.Comment),
                CustomTags(src.CustomTags),
                ReporteeId(src.ReporteeId)
            {}

            ~ReportPlayerClientRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["Comment"], Comment);
                FromJsonUtilS(input["CustomTags"], CustomTags);
                FromJsonUtilS(input["ReporteeId"], ReporteeId);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_Comment; ToJsonUtilS(Comment, each_Comment); output["Comment"] = each_Comment;
                Json::Value each_CustomTags; ToJsonUtilS(CustomTags, each_CustomTags); output["CustomTags"] = each_CustomTags;
                Json::Value each_ReporteeId; ToJsonUtilS(ReporteeId, each_ReporteeId); output["ReporteeId"] = each_ReporteeId;
                return output;
            }
        };

        struct ReportPlayerClientResult : public PlayFabResultCommon
        {
            Int32 SubmissionsRemaining;

            ReportPlayerClientResult() :
                PlayFabResultCommon(),
                SubmissionsRemaining()
            {}

            ReportPlayerClientResult(const ReportPlayerClientResult& src) :
                PlayFabResultCommon(),
                SubmissionsRemaining(src.SubmissionsRemaining)
            {}

            ~ReportPlayerClientResult() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilP(input["SubmissionsRemaining"], SubmissionsRemaining);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_SubmissionsRemaining; ToJsonUtilP(SubmissionsRemaining, each_SubmissionsRemaining); output["SubmissionsRemaining"] = each_SubmissionsRemaining;
                return output;
            }
        };

        struct RestoreIOSPurchasesRequest : public PlayFabRequestCommon
        {
            std::string CatalogVersion;
            std::map<std::string, std::string> CustomTags;
            std::string ReceiptData;

            RestoreIOSPurchasesRequest() :
                PlayFabRequestCommon(),
                CatalogVersion(),
                CustomTags(),
                ReceiptData()
            {}

            RestoreIOSPurchasesRequest(const RestoreIOSPurchasesRequest& src) :
                PlayFabRequestCommon(),
                CatalogVersion(src.CatalogVersion),
                CustomTags(src.CustomTags),
                ReceiptData(src.ReceiptData)
            {}

            ~RestoreIOSPurchasesRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["CatalogVersion"], CatalogVersion);
                FromJsonUtilS(input["CustomTags"], CustomTags);
                FromJsonUtilS(input["ReceiptData"], ReceiptData);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_CatalogVersion; ToJsonUtilS(CatalogVersion, each_CatalogVersion); output["CatalogVersion"] = each_CatalogVersion;
                Json::Value each_CustomTags; ToJsonUtilS(CustomTags, each_CustomTags); output["CustomTags"] = each_CustomTags;
                Json::Value each_ReceiptData; ToJsonUtilS(ReceiptData, each_ReceiptData); output["ReceiptData"] = each_ReceiptData;
                return output;
            }
        };

        struct RestoreIOSPurchasesResult : public PlayFabResultCommon
        {
            std::list<PurchaseReceiptFulfillment> Fulfillments;

            RestoreIOSPurchasesResult() :
                PlayFabResultCommon(),
                Fulfillments()
            {}

            RestoreIOSPurchasesResult(const RestoreIOSPurchasesResult& src) :
                PlayFabResultCommon(),
                Fulfillments(src.Fulfillments)
            {}

            ~RestoreIOSPurchasesResult() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilO(input["Fulfillments"], Fulfillments);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_Fulfillments; ToJsonUtilO(Fulfillments, each_Fulfillments); output["Fulfillments"] = each_Fulfillments;
                return output;
            }
        };

        struct RewardAdActivityRequest : public PlayFabRequestCommon
        {
            std::map<std::string, std::string> CustomTags;
            std::string PlacementId;
            std::string RewardId;

            RewardAdActivityRequest() :
                PlayFabRequestCommon(),
                CustomTags(),
                PlacementId(),
                RewardId()
            {}

            RewardAdActivityRequest(const RewardAdActivityRequest& src) :
                PlayFabRequestCommon(),
                CustomTags(src.CustomTags),
                PlacementId(src.PlacementId),
                RewardId(src.RewardId)
            {}

            ~RewardAdActivityRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["CustomTags"], CustomTags);
                FromJsonUtilS(input["PlacementId"], PlacementId);
                FromJsonUtilS(input["RewardId"], RewardId);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_CustomTags; ToJsonUtilS(CustomTags, each_CustomTags); output["CustomTags"] = each_CustomTags;
                Json::Value each_PlacementId; ToJsonUtilS(PlacementId, each_PlacementId); output["PlacementId"] = each_PlacementId;
                Json::Value each_RewardId; ToJsonUtilS(RewardId, each_RewardId); output["RewardId"] = each_RewardId;
                return output;
            }
        };

        struct RewardAdActivityResult : public PlayFabResultCommon
        {
            std::string AdActivityEventId;
            std::list<std::string> DebugResults;
            std::string PlacementId;
            std::string PlacementName;
            Boxed<Int32> PlacementViewsRemaining;
            Boxed<double> PlacementViewsResetMinutes;
            Boxed<AdRewardResults> RewardResults;

            RewardAdActivityResult() :
                PlayFabResultCommon(),
                AdActivityEventId(),
                DebugResults(),
                PlacementId(),
                PlacementName(),
                PlacementViewsRemaining(),
                PlacementViewsResetMinutes(),
                RewardResults()
            {}

            RewardAdActivityResult(const RewardAdActivityResult& src) :
                PlayFabResultCommon(),
                AdActivityEventId(src.AdActivityEventId),
                DebugResults(src.DebugResults),
                PlacementId(src.PlacementId),
                PlacementName(src.PlacementName),
                PlacementViewsRemaining(src.PlacementViewsRemaining),
                PlacementViewsResetMinutes(src.PlacementViewsResetMinutes),
                RewardResults(src.RewardResults)
            {}

            ~RewardAdActivityResult() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["AdActivityEventId"], AdActivityEventId);
                FromJsonUtilS(input["DebugResults"], DebugResults);
                FromJsonUtilS(input["PlacementId"], PlacementId);
                FromJsonUtilS(input["PlacementName"], PlacementName);
                FromJsonUtilP(input["PlacementViewsRemaining"], PlacementViewsRemaining);
                FromJsonUtilP(input["PlacementViewsResetMinutes"], PlacementViewsResetMinutes);
                FromJsonUtilO(input["RewardResults"], RewardResults);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_AdActivityEventId; ToJsonUtilS(AdActivityEventId, each_AdActivityEventId); output["AdActivityEventId"] = each_AdActivityEventId;
                Json::Value each_DebugResults; ToJsonUtilS(DebugResults, each_DebugResults); output["DebugResults"] = each_DebugResults;
                Json::Value each_PlacementId; ToJsonUtilS(PlacementId, each_PlacementId); output["PlacementId"] = each_PlacementId;
                Json::Value each_PlacementName; ToJsonUtilS(PlacementName, each_PlacementName); output["PlacementName"] = each_PlacementName;
                Json::Value each_PlacementViewsRemaining; ToJsonUtilP(PlacementViewsRemaining, each_PlacementViewsRemaining); output["PlacementViewsRemaining"] = each_PlacementViewsRemaining;
                Json::Value each_PlacementViewsResetMinutes; ToJsonUtilP(PlacementViewsResetMinutes, each_PlacementViewsResetMinutes); output["PlacementViewsResetMinutes"] = each_PlacementViewsResetMinutes;
                Json::Value each_RewardResults; ToJsonUtilO(RewardResults, each_RewardResults); output["RewardResults"] = each_RewardResults;
                return output;
            }
        };

        struct SendAccountRecoveryEmailRequest : public PlayFabRequestCommon
        {
            std::map<std::string, std::string> CustomTags;
            std::string Email;
            std::string EmailTemplateId;
            std::string TitleId;

            SendAccountRecoveryEmailRequest() :
                PlayFabRequestCommon(),
                CustomTags(),
                Email(),
                EmailTemplateId(),
                TitleId()
            {}

            SendAccountRecoveryEmailRequest(const SendAccountRecoveryEmailRequest& src) :
                PlayFabRequestCommon(),
                CustomTags(src.CustomTags),
                Email(src.Email),
                EmailTemplateId(src.EmailTemplateId),
                TitleId(src.TitleId)
            {}

            ~SendAccountRecoveryEmailRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["CustomTags"], CustomTags);
                FromJsonUtilS(input["Email"], Email);
                FromJsonUtilS(input["EmailTemplateId"], EmailTemplateId);
                FromJsonUtilS(input["TitleId"], TitleId);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_CustomTags; ToJsonUtilS(CustomTags, each_CustomTags); output["CustomTags"] = each_CustomTags;
                Json::Value each_Email; ToJsonUtilS(Email, each_Email); output["Email"] = each_Email;
                Json::Value each_EmailTemplateId; ToJsonUtilS(EmailTemplateId, each_EmailTemplateId); output["EmailTemplateId"] = each_EmailTemplateId;
                Json::Value each_TitleId; ToJsonUtilS(TitleId, each_TitleId); output["TitleId"] = each_TitleId;
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

        struct SetFriendTagsRequest : public PlayFabRequestCommon
        {
            std::string FriendPlayFabId;
            std::list<std::string> Tags;

            SetFriendTagsRequest() :
                PlayFabRequestCommon(),
                FriendPlayFabId(),
                Tags()
            {}

            SetFriendTagsRequest(const SetFriendTagsRequest& src) :
                PlayFabRequestCommon(),
                FriendPlayFabId(src.FriendPlayFabId),
                Tags(src.Tags)
            {}

            ~SetFriendTagsRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["FriendPlayFabId"], FriendPlayFabId);
                FromJsonUtilS(input["Tags"], Tags);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_FriendPlayFabId; ToJsonUtilS(FriendPlayFabId, each_FriendPlayFabId); output["FriendPlayFabId"] = each_FriendPlayFabId;
                Json::Value each_Tags; ToJsonUtilS(Tags, each_Tags); output["Tags"] = each_Tags;
                return output;
            }
        };

        struct SetFriendTagsResult : public PlayFabResultCommon
        {

            SetFriendTagsResult() :
                PlayFabResultCommon()
            {}

            SetFriendTagsResult(const SetFriendTagsResult&) :
                PlayFabResultCommon()
            {}

            ~SetFriendTagsResult() = default;

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
            std::string EncryptedRequest;
            std::string PlayerSecret;

            SetPlayerSecretRequest() :
                PlayFabRequestCommon(),
                EncryptedRequest(),
                PlayerSecret()
            {}

            SetPlayerSecretRequest(const SetPlayerSecretRequest& src) :
                PlayFabRequestCommon(),
                EncryptedRequest(src.EncryptedRequest),
                PlayerSecret(src.PlayerSecret)
            {}

            ~SetPlayerSecretRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["EncryptedRequest"], EncryptedRequest);
                FromJsonUtilS(input["PlayerSecret"], PlayerSecret);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_EncryptedRequest; ToJsonUtilS(EncryptedRequest, each_EncryptedRequest); output["EncryptedRequest"] = each_EncryptedRequest;
                Json::Value each_PlayerSecret; ToJsonUtilS(PlayerSecret, each_PlayerSecret); output["PlayerSecret"] = each_PlayerSecret;
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

        struct StartGameRequest : public PlayFabRequestCommon
        {
            std::string BuildVersion;
            std::string CharacterId;
            std::string CustomCommandLineData;
            std::map<std::string, std::string> CustomTags;
            std::string GameMode;
            Region pfRegion;
            std::string StatisticName;

            StartGameRequest() :
                PlayFabRequestCommon(),
                BuildVersion(),
                CharacterId(),
                CustomCommandLineData(),
                CustomTags(),
                GameMode(),
                pfRegion(),
                StatisticName()
            {}

            StartGameRequest(const StartGameRequest& src) :
                PlayFabRequestCommon(),
                BuildVersion(src.BuildVersion),
                CharacterId(src.CharacterId),
                CustomCommandLineData(src.CustomCommandLineData),
                CustomTags(src.CustomTags),
                GameMode(src.GameMode),
                pfRegion(src.pfRegion),
                StatisticName(src.StatisticName)
            {}

            ~StartGameRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["BuildVersion"], BuildVersion);
                FromJsonUtilS(input["CharacterId"], CharacterId);
                FromJsonUtilS(input["CustomCommandLineData"], CustomCommandLineData);
                FromJsonUtilS(input["CustomTags"], CustomTags);
                FromJsonUtilS(input["GameMode"], GameMode);
                FromJsonEnum(input["Region"], pfRegion);
                FromJsonUtilS(input["StatisticName"], StatisticName);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_BuildVersion; ToJsonUtilS(BuildVersion, each_BuildVersion); output["BuildVersion"] = each_BuildVersion;
                Json::Value each_CharacterId; ToJsonUtilS(CharacterId, each_CharacterId); output["CharacterId"] = each_CharacterId;
                Json::Value each_CustomCommandLineData; ToJsonUtilS(CustomCommandLineData, each_CustomCommandLineData); output["CustomCommandLineData"] = each_CustomCommandLineData;
                Json::Value each_CustomTags; ToJsonUtilS(CustomTags, each_CustomTags); output["CustomTags"] = each_CustomTags;
                Json::Value each_GameMode; ToJsonUtilS(GameMode, each_GameMode); output["GameMode"] = each_GameMode;
                Json::Value each_pfRegion; ToJsonEnum(pfRegion, each_pfRegion); output["Region"] = each_pfRegion;
                Json::Value each_StatisticName; ToJsonUtilS(StatisticName, each_StatisticName); output["StatisticName"] = each_StatisticName;
                return output;
            }
        };

        struct StartGameResult : public PlayFabResultCommon
        {
            std::string Expires;
            std::string LobbyID;
            std::string Password;
            std::string ServerIPV4Address;
            std::string ServerIPV6Address;
            Boxed<Int32> ServerPort;
            std::string ServerPublicDNSName;
            std::string Ticket;

            StartGameResult() :
                PlayFabResultCommon(),
                Expires(),
                LobbyID(),
                Password(),
                ServerIPV4Address(),
                ServerIPV6Address(),
                ServerPort(),
                ServerPublicDNSName(),
                Ticket()
            {}

            StartGameResult(const StartGameResult& src) :
                PlayFabResultCommon(),
                Expires(src.Expires),
                LobbyID(src.LobbyID),
                Password(src.Password),
                ServerIPV4Address(src.ServerIPV4Address),
                ServerIPV6Address(src.ServerIPV6Address),
                ServerPort(src.ServerPort),
                ServerPublicDNSName(src.ServerPublicDNSName),
                Ticket(src.Ticket)
            {}

            ~StartGameResult() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["Expires"], Expires);
                FromJsonUtilS(input["LobbyID"], LobbyID);
                FromJsonUtilS(input["Password"], Password);
                FromJsonUtilS(input["ServerIPV4Address"], ServerIPV4Address);
                FromJsonUtilS(input["ServerIPV6Address"], ServerIPV6Address);
                FromJsonUtilP(input["ServerPort"], ServerPort);
                FromJsonUtilS(input["ServerPublicDNSName"], ServerPublicDNSName);
                FromJsonUtilS(input["Ticket"], Ticket);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_Expires; ToJsonUtilS(Expires, each_Expires); output["Expires"] = each_Expires;
                Json::Value each_LobbyID; ToJsonUtilS(LobbyID, each_LobbyID); output["LobbyID"] = each_LobbyID;
                Json::Value each_Password; ToJsonUtilS(Password, each_Password); output["Password"] = each_Password;
                Json::Value each_ServerIPV4Address; ToJsonUtilS(ServerIPV4Address, each_ServerIPV4Address); output["ServerIPV4Address"] = each_ServerIPV4Address;
                Json::Value each_ServerIPV6Address; ToJsonUtilS(ServerIPV6Address, each_ServerIPV6Address); output["ServerIPV6Address"] = each_ServerIPV6Address;
                Json::Value each_ServerPort; ToJsonUtilP(ServerPort, each_ServerPort); output["ServerPort"] = each_ServerPort;
                Json::Value each_ServerPublicDNSName; ToJsonUtilS(ServerPublicDNSName, each_ServerPublicDNSName); output["ServerPublicDNSName"] = each_ServerPublicDNSName;
                Json::Value each_Ticket; ToJsonUtilS(Ticket, each_Ticket); output["Ticket"] = each_Ticket;
                return output;
            }
        };

        struct StartPurchaseRequest : public PlayFabRequestCommon
        {
            std::string CatalogVersion;
            std::map<std::string, std::string> CustomTags;
            std::list<ItemPurchaseRequest> Items;
            std::string StoreId;

            StartPurchaseRequest() :
                PlayFabRequestCommon(),
                CatalogVersion(),
                CustomTags(),
                Items(),
                StoreId()
            {}

            StartPurchaseRequest(const StartPurchaseRequest& src) :
                PlayFabRequestCommon(),
                CatalogVersion(src.CatalogVersion),
                CustomTags(src.CustomTags),
                Items(src.Items),
                StoreId(src.StoreId)
            {}

            ~StartPurchaseRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["CatalogVersion"], CatalogVersion);
                FromJsonUtilS(input["CustomTags"], CustomTags);
                FromJsonUtilO(input["Items"], Items);
                FromJsonUtilS(input["StoreId"], StoreId);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_CatalogVersion; ToJsonUtilS(CatalogVersion, each_CatalogVersion); output["CatalogVersion"] = each_CatalogVersion;
                Json::Value each_CustomTags; ToJsonUtilS(CustomTags, each_CustomTags); output["CustomTags"] = each_CustomTags;
                Json::Value each_Items; ToJsonUtilO(Items, each_Items); output["Items"] = each_Items;
                Json::Value each_StoreId; ToJsonUtilS(StoreId, each_StoreId); output["StoreId"] = each_StoreId;
                return output;
            }
        };

        struct StartPurchaseResult : public PlayFabResultCommon
        {
            std::list<CartItem> Contents;
            std::string OrderId;
            std::list<PaymentOption> PaymentOptions;
            std::map<std::string, Int32> VirtualCurrencyBalances;

            StartPurchaseResult() :
                PlayFabResultCommon(),
                Contents(),
                OrderId(),
                PaymentOptions(),
                VirtualCurrencyBalances()
            {}

            StartPurchaseResult(const StartPurchaseResult& src) :
                PlayFabResultCommon(),
                Contents(src.Contents),
                OrderId(src.OrderId),
                PaymentOptions(src.PaymentOptions),
                VirtualCurrencyBalances(src.VirtualCurrencyBalances)
            {}

            ~StartPurchaseResult() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilO(input["Contents"], Contents);
                FromJsonUtilS(input["OrderId"], OrderId);
                FromJsonUtilO(input["PaymentOptions"], PaymentOptions);
                FromJsonUtilP(input["VirtualCurrencyBalances"], VirtualCurrencyBalances);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_Contents; ToJsonUtilO(Contents, each_Contents); output["Contents"] = each_Contents;
                Json::Value each_OrderId; ToJsonUtilS(OrderId, each_OrderId); output["OrderId"] = each_OrderId;
                Json::Value each_PaymentOptions; ToJsonUtilO(PaymentOptions, each_PaymentOptions); output["PaymentOptions"] = each_PaymentOptions;
                Json::Value each_VirtualCurrencyBalances; ToJsonUtilP(VirtualCurrencyBalances, each_VirtualCurrencyBalances); output["VirtualCurrencyBalances"] = each_VirtualCurrencyBalances;
                return output;
            }
        };

        struct StatisticUpdate : public PlayFabBaseModel
        {
            std::string StatisticName;
            Int32 Value;
            Boxed<Uint32> Version;

            StatisticUpdate() :
                PlayFabBaseModel(),
                StatisticName(),
                Value(),
                Version()
            {}

            StatisticUpdate(const StatisticUpdate& src) :
                PlayFabBaseModel(),
                StatisticName(src.StatisticName),
                Value(src.Value),
                Version(src.Version)
            {}

            ~StatisticUpdate() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["StatisticName"], StatisticName);
                FromJsonUtilP(input["Value"], Value);
                FromJsonUtilP(input["Version"], Version);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_StatisticName; ToJsonUtilS(StatisticName, each_StatisticName); output["StatisticName"] = each_StatisticName;
                Json::Value each_Value; ToJsonUtilP(Value, each_Value); output["Value"] = each_Value;
                Json::Value each_Version; ToJsonUtilP(Version, each_Version); output["Version"] = each_Version;
                return output;
            }
        };

        struct SubtractUserVirtualCurrencyRequest : public PlayFabRequestCommon
        {
            Int32 Amount;
            std::map<std::string, std::string> CustomTags;
            std::string VirtualCurrency;

            SubtractUserVirtualCurrencyRequest() :
                PlayFabRequestCommon(),
                Amount(),
                CustomTags(),
                VirtualCurrency()
            {}

            SubtractUserVirtualCurrencyRequest(const SubtractUserVirtualCurrencyRequest& src) :
                PlayFabRequestCommon(),
                Amount(src.Amount),
                CustomTags(src.CustomTags),
                VirtualCurrency(src.VirtualCurrency)
            {}

            ~SubtractUserVirtualCurrencyRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilP(input["Amount"], Amount);
                FromJsonUtilS(input["CustomTags"], CustomTags);
                FromJsonUtilS(input["VirtualCurrency"], VirtualCurrency);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_Amount; ToJsonUtilP(Amount, each_Amount); output["Amount"] = each_Amount;
                Json::Value each_CustomTags; ToJsonUtilS(CustomTags, each_CustomTags); output["CustomTags"] = each_CustomTags;
                Json::Value each_VirtualCurrency; ToJsonUtilS(VirtualCurrency, each_VirtualCurrency); output["VirtualCurrency"] = each_VirtualCurrency;
                return output;
            }
        };

        struct UnlinkAndroidDeviceIDRequest : public PlayFabRequestCommon
        {
            std::string AndroidDeviceId;
            std::map<std::string, std::string> CustomTags;

            UnlinkAndroidDeviceIDRequest() :
                PlayFabRequestCommon(),
                AndroidDeviceId(),
                CustomTags()
            {}

            UnlinkAndroidDeviceIDRequest(const UnlinkAndroidDeviceIDRequest& src) :
                PlayFabRequestCommon(),
                AndroidDeviceId(src.AndroidDeviceId),
                CustomTags(src.CustomTags)
            {}

            ~UnlinkAndroidDeviceIDRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["AndroidDeviceId"], AndroidDeviceId);
                FromJsonUtilS(input["CustomTags"], CustomTags);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_AndroidDeviceId; ToJsonUtilS(AndroidDeviceId, each_AndroidDeviceId); output["AndroidDeviceId"] = each_AndroidDeviceId;
                Json::Value each_CustomTags; ToJsonUtilS(CustomTags, each_CustomTags); output["CustomTags"] = each_CustomTags;
                return output;
            }
        };

        struct UnlinkAndroidDeviceIDResult : public PlayFabResultCommon
        {

            UnlinkAndroidDeviceIDResult() :
                PlayFabResultCommon()
            {}

            UnlinkAndroidDeviceIDResult(const UnlinkAndroidDeviceIDResult&) :
                PlayFabResultCommon()
            {}

            ~UnlinkAndroidDeviceIDResult() = default;

            void FromJson(const Json::Value&) override
            {
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                return output;
            }
        };

        struct UnlinkAppleRequest : public PlayFabRequestCommon
        {
            std::map<std::string, std::string> CustomTags;

            UnlinkAppleRequest() :
                PlayFabRequestCommon(),
                CustomTags()
            {}

            UnlinkAppleRequest(const UnlinkAppleRequest& src) :
                PlayFabRequestCommon(),
                CustomTags(src.CustomTags)
            {}

            ~UnlinkAppleRequest() = default;

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

        struct UnlinkCustomIDRequest : public PlayFabRequestCommon
        {
            std::string CustomId;
            std::map<std::string, std::string> CustomTags;

            UnlinkCustomIDRequest() :
                PlayFabRequestCommon(),
                CustomId(),
                CustomTags()
            {}

            UnlinkCustomIDRequest(const UnlinkCustomIDRequest& src) :
                PlayFabRequestCommon(),
                CustomId(src.CustomId),
                CustomTags(src.CustomTags)
            {}

            ~UnlinkCustomIDRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["CustomId"], CustomId);
                FromJsonUtilS(input["CustomTags"], CustomTags);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_CustomId; ToJsonUtilS(CustomId, each_CustomId); output["CustomId"] = each_CustomId;
                Json::Value each_CustomTags; ToJsonUtilS(CustomTags, each_CustomTags); output["CustomTags"] = each_CustomTags;
                return output;
            }
        };

        struct UnlinkCustomIDResult : public PlayFabResultCommon
        {

            UnlinkCustomIDResult() :
                PlayFabResultCommon()
            {}

            UnlinkCustomIDResult(const UnlinkCustomIDResult&) :
                PlayFabResultCommon()
            {}

            ~UnlinkCustomIDResult() = default;

            void FromJson(const Json::Value&) override
            {
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                return output;
            }
        };

        struct UnlinkFacebookAccountRequest : public PlayFabRequestCommon
        {
            std::map<std::string, std::string> CustomTags;

            UnlinkFacebookAccountRequest() :
                PlayFabRequestCommon(),
                CustomTags()
            {}

            UnlinkFacebookAccountRequest(const UnlinkFacebookAccountRequest& src) :
                PlayFabRequestCommon(),
                CustomTags(src.CustomTags)
            {}

            ~UnlinkFacebookAccountRequest() = default;

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

        struct UnlinkFacebookAccountResult : public PlayFabResultCommon
        {

            UnlinkFacebookAccountResult() :
                PlayFabResultCommon()
            {}

            UnlinkFacebookAccountResult(const UnlinkFacebookAccountResult&) :
                PlayFabResultCommon()
            {}

            ~UnlinkFacebookAccountResult() = default;

            void FromJson(const Json::Value&) override
            {
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                return output;
            }
        };

        struct UnlinkFacebookInstantGamesIdRequest : public PlayFabRequestCommon
        {
            std::map<std::string, std::string> CustomTags;
            std::string FacebookInstantGamesId;

            UnlinkFacebookInstantGamesIdRequest() :
                PlayFabRequestCommon(),
                CustomTags(),
                FacebookInstantGamesId()
            {}

            UnlinkFacebookInstantGamesIdRequest(const UnlinkFacebookInstantGamesIdRequest& src) :
                PlayFabRequestCommon(),
                CustomTags(src.CustomTags),
                FacebookInstantGamesId(src.FacebookInstantGamesId)
            {}

            ~UnlinkFacebookInstantGamesIdRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["CustomTags"], CustomTags);
                FromJsonUtilS(input["FacebookInstantGamesId"], FacebookInstantGamesId);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_CustomTags; ToJsonUtilS(CustomTags, each_CustomTags); output["CustomTags"] = each_CustomTags;
                Json::Value each_FacebookInstantGamesId; ToJsonUtilS(FacebookInstantGamesId, each_FacebookInstantGamesId); output["FacebookInstantGamesId"] = each_FacebookInstantGamesId;
                return output;
            }
        };

        struct UnlinkFacebookInstantGamesIdResult : public PlayFabResultCommon
        {

            UnlinkFacebookInstantGamesIdResult() :
                PlayFabResultCommon()
            {}

            UnlinkFacebookInstantGamesIdResult(const UnlinkFacebookInstantGamesIdResult&) :
                PlayFabResultCommon()
            {}

            ~UnlinkFacebookInstantGamesIdResult() = default;

            void FromJson(const Json::Value&) override
            {
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                return output;
            }
        };

        struct UnlinkGameCenterAccountRequest : public PlayFabRequestCommon
        {
            std::map<std::string, std::string> CustomTags;

            UnlinkGameCenterAccountRequest() :
                PlayFabRequestCommon(),
                CustomTags()
            {}

            UnlinkGameCenterAccountRequest(const UnlinkGameCenterAccountRequest& src) :
                PlayFabRequestCommon(),
                CustomTags(src.CustomTags)
            {}

            ~UnlinkGameCenterAccountRequest() = default;

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

        struct UnlinkGameCenterAccountResult : public PlayFabResultCommon
        {

            UnlinkGameCenterAccountResult() :
                PlayFabResultCommon()
            {}

            UnlinkGameCenterAccountResult(const UnlinkGameCenterAccountResult&) :
                PlayFabResultCommon()
            {}

            ~UnlinkGameCenterAccountResult() = default;

            void FromJson(const Json::Value&) override
            {
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                return output;
            }
        };

        struct UnlinkGoogleAccountRequest : public PlayFabRequestCommon
        {
            std::map<std::string, std::string> CustomTags;

            UnlinkGoogleAccountRequest() :
                PlayFabRequestCommon(),
                CustomTags()
            {}

            UnlinkGoogleAccountRequest(const UnlinkGoogleAccountRequest& src) :
                PlayFabRequestCommon(),
                CustomTags(src.CustomTags)
            {}

            ~UnlinkGoogleAccountRequest() = default;

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

        struct UnlinkGoogleAccountResult : public PlayFabResultCommon
        {

            UnlinkGoogleAccountResult() :
                PlayFabResultCommon()
            {}

            UnlinkGoogleAccountResult(const UnlinkGoogleAccountResult&) :
                PlayFabResultCommon()
            {}

            ~UnlinkGoogleAccountResult() = default;

            void FromJson(const Json::Value&) override
            {
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                return output;
            }
        };

        struct UnlinkIOSDeviceIDRequest : public PlayFabRequestCommon
        {
            std::map<std::string, std::string> CustomTags;
            std::string DeviceId;

            UnlinkIOSDeviceIDRequest() :
                PlayFabRequestCommon(),
                CustomTags(),
                DeviceId()
            {}

            UnlinkIOSDeviceIDRequest(const UnlinkIOSDeviceIDRequest& src) :
                PlayFabRequestCommon(),
                CustomTags(src.CustomTags),
                DeviceId(src.DeviceId)
            {}

            ~UnlinkIOSDeviceIDRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["CustomTags"], CustomTags);
                FromJsonUtilS(input["DeviceId"], DeviceId);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_CustomTags; ToJsonUtilS(CustomTags, each_CustomTags); output["CustomTags"] = each_CustomTags;
                Json::Value each_DeviceId; ToJsonUtilS(DeviceId, each_DeviceId); output["DeviceId"] = each_DeviceId;
                return output;
            }
        };

        struct UnlinkIOSDeviceIDResult : public PlayFabResultCommon
        {

            UnlinkIOSDeviceIDResult() :
                PlayFabResultCommon()
            {}

            UnlinkIOSDeviceIDResult(const UnlinkIOSDeviceIDResult&) :
                PlayFabResultCommon()
            {}

            ~UnlinkIOSDeviceIDResult() = default;

            void FromJson(const Json::Value&) override
            {
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                return output;
            }
        };

        struct UnlinkKongregateAccountRequest : public PlayFabRequestCommon
        {
            std::map<std::string, std::string> CustomTags;

            UnlinkKongregateAccountRequest() :
                PlayFabRequestCommon(),
                CustomTags()
            {}

            UnlinkKongregateAccountRequest(const UnlinkKongregateAccountRequest& src) :
                PlayFabRequestCommon(),
                CustomTags(src.CustomTags)
            {}

            ~UnlinkKongregateAccountRequest() = default;

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

        struct UnlinkKongregateAccountResult : public PlayFabResultCommon
        {

            UnlinkKongregateAccountResult() :
                PlayFabResultCommon()
            {}

            UnlinkKongregateAccountResult(const UnlinkKongregateAccountResult&) :
                PlayFabResultCommon()
            {}

            ~UnlinkKongregateAccountResult() = default;

            void FromJson(const Json::Value&) override
            {
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                return output;
            }
        };

        struct UnlinkNintendoServiceAccountRequest : public PlayFabRequestCommon
        {
            std::map<std::string, std::string> CustomTags;

            UnlinkNintendoServiceAccountRequest() :
                PlayFabRequestCommon(),
                CustomTags()
            {}

            UnlinkNintendoServiceAccountRequest(const UnlinkNintendoServiceAccountRequest& src) :
                PlayFabRequestCommon(),
                CustomTags(src.CustomTags)
            {}

            ~UnlinkNintendoServiceAccountRequest() = default;

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

        struct UnlinkNintendoSwitchDeviceIdRequest : public PlayFabRequestCommon
        {
            std::map<std::string, std::string> CustomTags;
            std::string NintendoSwitchDeviceId;

            UnlinkNintendoSwitchDeviceIdRequest() :
                PlayFabRequestCommon(),
                CustomTags(),
                NintendoSwitchDeviceId()
            {}

            UnlinkNintendoSwitchDeviceIdRequest(const UnlinkNintendoSwitchDeviceIdRequest& src) :
                PlayFabRequestCommon(),
                CustomTags(src.CustomTags),
                NintendoSwitchDeviceId(src.NintendoSwitchDeviceId)
            {}

            ~UnlinkNintendoSwitchDeviceIdRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["CustomTags"], CustomTags);
                FromJsonUtilS(input["NintendoSwitchDeviceId"], NintendoSwitchDeviceId);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_CustomTags; ToJsonUtilS(CustomTags, each_CustomTags); output["CustomTags"] = each_CustomTags;
                Json::Value each_NintendoSwitchDeviceId; ToJsonUtilS(NintendoSwitchDeviceId, each_NintendoSwitchDeviceId); output["NintendoSwitchDeviceId"] = each_NintendoSwitchDeviceId;
                return output;
            }
        };

        struct UnlinkNintendoSwitchDeviceIdResult : public PlayFabResultCommon
        {

            UnlinkNintendoSwitchDeviceIdResult() :
                PlayFabResultCommon()
            {}

            UnlinkNintendoSwitchDeviceIdResult(const UnlinkNintendoSwitchDeviceIdResult&) :
                PlayFabResultCommon()
            {}

            ~UnlinkNintendoSwitchDeviceIdResult() = default;

            void FromJson(const Json::Value&) override
            {
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                return output;
            }
        };

        struct UnlinkOpenIdConnectRequest : public PlayFabRequestCommon
        {
            std::string ConnectionId;
            std::map<std::string, std::string> CustomTags;

            UnlinkOpenIdConnectRequest() :
                PlayFabRequestCommon(),
                ConnectionId(),
                CustomTags()
            {}

            UnlinkOpenIdConnectRequest(const UnlinkOpenIdConnectRequest& src) :
                PlayFabRequestCommon(),
                ConnectionId(src.ConnectionId),
                CustomTags(src.CustomTags)
            {}

            ~UnlinkOpenIdConnectRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["ConnectionId"], ConnectionId);
                FromJsonUtilS(input["CustomTags"], CustomTags);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_ConnectionId; ToJsonUtilS(ConnectionId, each_ConnectionId); output["ConnectionId"] = each_ConnectionId;
                Json::Value each_CustomTags; ToJsonUtilS(CustomTags, each_CustomTags); output["CustomTags"] = each_CustomTags;
                return output;
            }
        };

        struct UnlinkPSNAccountRequest : public PlayFabRequestCommon
        {
            std::map<std::string, std::string> CustomTags;

            UnlinkPSNAccountRequest() :
                PlayFabRequestCommon(),
                CustomTags()
            {}

            UnlinkPSNAccountRequest(const UnlinkPSNAccountRequest& src) :
                PlayFabRequestCommon(),
                CustomTags(src.CustomTags)
            {}

            ~UnlinkPSNAccountRequest() = default;

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

        struct UnlinkPSNAccountResult : public PlayFabResultCommon
        {

            UnlinkPSNAccountResult() :
                PlayFabResultCommon()
            {}

            UnlinkPSNAccountResult(const UnlinkPSNAccountResult&) :
                PlayFabResultCommon()
            {}

            ~UnlinkPSNAccountResult() = default;

            void FromJson(const Json::Value&) override
            {
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                return output;
            }
        };

        struct UnlinkSteamAccountRequest : public PlayFabRequestCommon
        {
            std::map<std::string, std::string> CustomTags;

            UnlinkSteamAccountRequest() :
                PlayFabRequestCommon(),
                CustomTags()
            {}

            UnlinkSteamAccountRequest(const UnlinkSteamAccountRequest& src) :
                PlayFabRequestCommon(),
                CustomTags(src.CustomTags)
            {}

            ~UnlinkSteamAccountRequest() = default;

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

        struct UnlinkSteamAccountResult : public PlayFabResultCommon
        {

            UnlinkSteamAccountResult() :
                PlayFabResultCommon()
            {}

            UnlinkSteamAccountResult(const UnlinkSteamAccountResult&) :
                PlayFabResultCommon()
            {}

            ~UnlinkSteamAccountResult() = default;

            void FromJson(const Json::Value&) override
            {
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                return output;
            }
        };

        struct UnlinkTwitchAccountRequest : public PlayFabRequestCommon
        {
            std::string AccessToken;
            std::map<std::string, std::string> CustomTags;

            UnlinkTwitchAccountRequest() :
                PlayFabRequestCommon(),
                AccessToken(),
                CustomTags()
            {}

            UnlinkTwitchAccountRequest(const UnlinkTwitchAccountRequest& src) :
                PlayFabRequestCommon(),
                AccessToken(src.AccessToken),
                CustomTags(src.CustomTags)
            {}

            ~UnlinkTwitchAccountRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["AccessToken"], AccessToken);
                FromJsonUtilS(input["CustomTags"], CustomTags);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_AccessToken; ToJsonUtilS(AccessToken, each_AccessToken); output["AccessToken"] = each_AccessToken;
                Json::Value each_CustomTags; ToJsonUtilS(CustomTags, each_CustomTags); output["CustomTags"] = each_CustomTags;
                return output;
            }
        };

        struct UnlinkTwitchAccountResult : public PlayFabResultCommon
        {

            UnlinkTwitchAccountResult() :
                PlayFabResultCommon()
            {}

            UnlinkTwitchAccountResult(const UnlinkTwitchAccountResult&) :
                PlayFabResultCommon()
            {}

            ~UnlinkTwitchAccountResult() = default;

            void FromJson(const Json::Value&) override
            {
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                return output;
            }
        };

        struct UnlinkXboxAccountRequest : public PlayFabRequestCommon
        {
            std::map<std::string, std::string> CustomTags;

            UnlinkXboxAccountRequest() :
                PlayFabRequestCommon(),
                CustomTags()
            {}

            UnlinkXboxAccountRequest(const UnlinkXboxAccountRequest& src) :
                PlayFabRequestCommon(),
                CustomTags(src.CustomTags)
            {}

            ~UnlinkXboxAccountRequest() = default;

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

        struct UnlinkXboxAccountResult : public PlayFabResultCommon
        {

            UnlinkXboxAccountResult() :
                PlayFabResultCommon()
            {}

            UnlinkXboxAccountResult(const UnlinkXboxAccountResult&) :
                PlayFabResultCommon()
            {}

            ~UnlinkXboxAccountResult() = default;

            void FromJson(const Json::Value&) override
            {
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                return output;
            }
        };

        struct UnlockContainerInstanceRequest : public PlayFabRequestCommon
        {
            std::string CatalogVersion;
            std::string CharacterId;
            std::string ContainerItemInstanceId;
            std::map<std::string, std::string> CustomTags;
            std::string KeyItemInstanceId;

            UnlockContainerInstanceRequest() :
                PlayFabRequestCommon(),
                CatalogVersion(),
                CharacterId(),
                ContainerItemInstanceId(),
                CustomTags(),
                KeyItemInstanceId()
            {}

            UnlockContainerInstanceRequest(const UnlockContainerInstanceRequest& src) :
                PlayFabRequestCommon(),
                CatalogVersion(src.CatalogVersion),
                CharacterId(src.CharacterId),
                ContainerItemInstanceId(src.ContainerItemInstanceId),
                CustomTags(src.CustomTags),
                KeyItemInstanceId(src.KeyItemInstanceId)
            {}

            ~UnlockContainerInstanceRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["CatalogVersion"], CatalogVersion);
                FromJsonUtilS(input["CharacterId"], CharacterId);
                FromJsonUtilS(input["ContainerItemInstanceId"], ContainerItemInstanceId);
                FromJsonUtilS(input["CustomTags"], CustomTags);
                FromJsonUtilS(input["KeyItemInstanceId"], KeyItemInstanceId);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_CatalogVersion; ToJsonUtilS(CatalogVersion, each_CatalogVersion); output["CatalogVersion"] = each_CatalogVersion;
                Json::Value each_CharacterId; ToJsonUtilS(CharacterId, each_CharacterId); output["CharacterId"] = each_CharacterId;
                Json::Value each_ContainerItemInstanceId; ToJsonUtilS(ContainerItemInstanceId, each_ContainerItemInstanceId); output["ContainerItemInstanceId"] = each_ContainerItemInstanceId;
                Json::Value each_CustomTags; ToJsonUtilS(CustomTags, each_CustomTags); output["CustomTags"] = each_CustomTags;
                Json::Value each_KeyItemInstanceId; ToJsonUtilS(KeyItemInstanceId, each_KeyItemInstanceId); output["KeyItemInstanceId"] = each_KeyItemInstanceId;
                return output;
            }
        };

        struct UnlockContainerItemRequest : public PlayFabRequestCommon
        {
            std::string CatalogVersion;
            std::string CharacterId;
            std::string ContainerItemId;
            std::map<std::string, std::string> CustomTags;

            UnlockContainerItemRequest() :
                PlayFabRequestCommon(),
                CatalogVersion(),
                CharacterId(),
                ContainerItemId(),
                CustomTags()
            {}

            UnlockContainerItemRequest(const UnlockContainerItemRequest& src) :
                PlayFabRequestCommon(),
                CatalogVersion(src.CatalogVersion),
                CharacterId(src.CharacterId),
                ContainerItemId(src.ContainerItemId),
                CustomTags(src.CustomTags)
            {}

            ~UnlockContainerItemRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["CatalogVersion"], CatalogVersion);
                FromJsonUtilS(input["CharacterId"], CharacterId);
                FromJsonUtilS(input["ContainerItemId"], ContainerItemId);
                FromJsonUtilS(input["CustomTags"], CustomTags);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_CatalogVersion; ToJsonUtilS(CatalogVersion, each_CatalogVersion); output["CatalogVersion"] = each_CatalogVersion;
                Json::Value each_CharacterId; ToJsonUtilS(CharacterId, each_CharacterId); output["CharacterId"] = each_CharacterId;
                Json::Value each_ContainerItemId; ToJsonUtilS(ContainerItemId, each_ContainerItemId); output["ContainerItemId"] = each_ContainerItemId;
                Json::Value each_CustomTags; ToJsonUtilS(CustomTags, each_CustomTags); output["CustomTags"] = each_CustomTags;
                return output;
            }
        };

        struct UnlockContainerItemResult : public PlayFabResultCommon
        {
            std::list<ItemInstance> GrantedItems;
            std::string UnlockedItemInstanceId;
            std::string UnlockedWithItemInstanceId;
            std::map<std::string, Uint32> VirtualCurrency;

            UnlockContainerItemResult() :
                PlayFabResultCommon(),
                GrantedItems(),
                UnlockedItemInstanceId(),
                UnlockedWithItemInstanceId(),
                VirtualCurrency()
            {}

            UnlockContainerItemResult(const UnlockContainerItemResult& src) :
                PlayFabResultCommon(),
                GrantedItems(src.GrantedItems),
                UnlockedItemInstanceId(src.UnlockedItemInstanceId),
                UnlockedWithItemInstanceId(src.UnlockedWithItemInstanceId),
                VirtualCurrency(src.VirtualCurrency)
            {}

            ~UnlockContainerItemResult() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilO(input["GrantedItems"], GrantedItems);
                FromJsonUtilS(input["UnlockedItemInstanceId"], UnlockedItemInstanceId);
                FromJsonUtilS(input["UnlockedWithItemInstanceId"], UnlockedWithItemInstanceId);
                FromJsonUtilP(input["VirtualCurrency"], VirtualCurrency);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_GrantedItems; ToJsonUtilO(GrantedItems, each_GrantedItems); output["GrantedItems"] = each_GrantedItems;
                Json::Value each_UnlockedItemInstanceId; ToJsonUtilS(UnlockedItemInstanceId, each_UnlockedItemInstanceId); output["UnlockedItemInstanceId"] = each_UnlockedItemInstanceId;
                Json::Value each_UnlockedWithItemInstanceId; ToJsonUtilS(UnlockedWithItemInstanceId, each_UnlockedWithItemInstanceId); output["UnlockedWithItemInstanceId"] = each_UnlockedWithItemInstanceId;
                Json::Value each_VirtualCurrency; ToJsonUtilP(VirtualCurrency, each_VirtualCurrency); output["VirtualCurrency"] = each_VirtualCurrency;
                return output;
            }
        };

        struct UpdateAvatarUrlRequest : public PlayFabRequestCommon
        {
            std::string ImageUrl;

            UpdateAvatarUrlRequest() :
                PlayFabRequestCommon(),
                ImageUrl()
            {}

            UpdateAvatarUrlRequest(const UpdateAvatarUrlRequest& src) :
                PlayFabRequestCommon(),
                ImageUrl(src.ImageUrl)
            {}

            ~UpdateAvatarUrlRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["ImageUrl"], ImageUrl);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_ImageUrl; ToJsonUtilS(ImageUrl, each_ImageUrl); output["ImageUrl"] = each_ImageUrl;
                return output;
            }
        };

        struct UpdateCharacterDataRequest : public PlayFabRequestCommon
        {
            std::string CharacterId;
            std::map<std::string, std::string> CustomTags;
            std::map<std::string, std::string> Data;
            std::list<std::string> KeysToRemove;
            Boxed<UserDataPermission> Permission;

            UpdateCharacterDataRequest() :
                PlayFabRequestCommon(),
                CharacterId(),
                CustomTags(),
                Data(),
                KeysToRemove(),
                Permission()
            {}

            UpdateCharacterDataRequest(const UpdateCharacterDataRequest& src) :
                PlayFabRequestCommon(),
                CharacterId(src.CharacterId),
                CustomTags(src.CustomTags),
                Data(src.Data),
                KeysToRemove(src.KeysToRemove),
                Permission(src.Permission)
            {}

            ~UpdateCharacterDataRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["CharacterId"], CharacterId);
                FromJsonUtilS(input["CustomTags"], CustomTags);
                FromJsonUtilS(input["Data"], Data);
                FromJsonUtilS(input["KeysToRemove"], KeysToRemove);
                FromJsonUtilE(input["Permission"], Permission);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_CharacterId; ToJsonUtilS(CharacterId, each_CharacterId); output["CharacterId"] = each_CharacterId;
                Json::Value each_CustomTags; ToJsonUtilS(CustomTags, each_CustomTags); output["CustomTags"] = each_CustomTags;
                Json::Value each_Data; ToJsonUtilS(Data, each_Data); output["Data"] = each_Data;
                Json::Value each_KeysToRemove; ToJsonUtilS(KeysToRemove, each_KeysToRemove); output["KeysToRemove"] = each_KeysToRemove;
                Json::Value each_Permission; ToJsonUtilE(Permission, each_Permission); output["Permission"] = each_Permission;
                return output;
            }
        };

        struct UpdateCharacterDataResult : public PlayFabResultCommon
        {
            Uint32 DataVersion;

            UpdateCharacterDataResult() :
                PlayFabResultCommon(),
                DataVersion()
            {}

            UpdateCharacterDataResult(const UpdateCharacterDataResult& src) :
                PlayFabResultCommon(),
                DataVersion(src.DataVersion)
            {}

            ~UpdateCharacterDataResult() = default;

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

        struct UpdateCharacterStatisticsRequest : public PlayFabRequestCommon
        {
            std::string CharacterId;
            std::map<std::string, Int32> CharacterStatistics;
            std::map<std::string, std::string> CustomTags;

            UpdateCharacterStatisticsRequest() :
                PlayFabRequestCommon(),
                CharacterId(),
                CharacterStatistics(),
                CustomTags()
            {}

            UpdateCharacterStatisticsRequest(const UpdateCharacterStatisticsRequest& src) :
                PlayFabRequestCommon(),
                CharacterId(src.CharacterId),
                CharacterStatistics(src.CharacterStatistics),
                CustomTags(src.CustomTags)
            {}

            ~UpdateCharacterStatisticsRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["CharacterId"], CharacterId);
                FromJsonUtilP(input["CharacterStatistics"], CharacterStatistics);
                FromJsonUtilS(input["CustomTags"], CustomTags);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_CharacterId; ToJsonUtilS(CharacterId, each_CharacterId); output["CharacterId"] = each_CharacterId;
                Json::Value each_CharacterStatistics; ToJsonUtilP(CharacterStatistics, each_CharacterStatistics); output["CharacterStatistics"] = each_CharacterStatistics;
                Json::Value each_CustomTags; ToJsonUtilS(CustomTags, each_CustomTags); output["CustomTags"] = each_CustomTags;
                return output;
            }
        };

        struct UpdateCharacterStatisticsResult : public PlayFabResultCommon
        {

            UpdateCharacterStatisticsResult() :
                PlayFabResultCommon()
            {}

            UpdateCharacterStatisticsResult(const UpdateCharacterStatisticsResult&) :
                PlayFabResultCommon()
            {}

            ~UpdateCharacterStatisticsResult() = default;

            void FromJson(const Json::Value&) override
            {
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                return output;
            }
        };

        struct UpdatePlayerStatisticsRequest : public PlayFabRequestCommon
        {
            std::map<std::string, std::string> CustomTags;
            std::list<StatisticUpdate> Statistics;

            UpdatePlayerStatisticsRequest() :
                PlayFabRequestCommon(),
                CustomTags(),
                Statistics()
            {}

            UpdatePlayerStatisticsRequest(const UpdatePlayerStatisticsRequest& src) :
                PlayFabRequestCommon(),
                CustomTags(src.CustomTags),
                Statistics(src.Statistics)
            {}

            ~UpdatePlayerStatisticsRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["CustomTags"], CustomTags);
                FromJsonUtilO(input["Statistics"], Statistics);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_CustomTags; ToJsonUtilS(CustomTags, each_CustomTags); output["CustomTags"] = each_CustomTags;
                Json::Value each_Statistics; ToJsonUtilO(Statistics, each_Statistics); output["Statistics"] = each_Statistics;
                return output;
            }
        };

        struct UpdatePlayerStatisticsResult : public PlayFabResultCommon
        {

            UpdatePlayerStatisticsResult() :
                PlayFabResultCommon()
            {}

            UpdatePlayerStatisticsResult(const UpdatePlayerStatisticsResult&) :
                PlayFabResultCommon()
            {}

            ~UpdatePlayerStatisticsResult() = default;

            void FromJson(const Json::Value&) override
            {
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                return output;
            }
        };

        struct UpdateSharedGroupDataRequest : public PlayFabRequestCommon
        {
            std::map<std::string, std::string> CustomTags;
            std::map<std::string, std::string> Data;
            std::list<std::string> KeysToRemove;
            Boxed<UserDataPermission> Permission;
            std::string SharedGroupId;

            UpdateSharedGroupDataRequest() :
                PlayFabRequestCommon(),
                CustomTags(),
                Data(),
                KeysToRemove(),
                Permission(),
                SharedGroupId()
            {}

            UpdateSharedGroupDataRequest(const UpdateSharedGroupDataRequest& src) :
                PlayFabRequestCommon(),
                CustomTags(src.CustomTags),
                Data(src.Data),
                KeysToRemove(src.KeysToRemove),
                Permission(src.Permission),
                SharedGroupId(src.SharedGroupId)
            {}

            ~UpdateSharedGroupDataRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["CustomTags"], CustomTags);
                FromJsonUtilS(input["Data"], Data);
                FromJsonUtilS(input["KeysToRemove"], KeysToRemove);
                FromJsonUtilE(input["Permission"], Permission);
                FromJsonUtilS(input["SharedGroupId"], SharedGroupId);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_CustomTags; ToJsonUtilS(CustomTags, each_CustomTags); output["CustomTags"] = each_CustomTags;
                Json::Value each_Data; ToJsonUtilS(Data, each_Data); output["Data"] = each_Data;
                Json::Value each_KeysToRemove; ToJsonUtilS(KeysToRemove, each_KeysToRemove); output["KeysToRemove"] = each_KeysToRemove;
                Json::Value each_Permission; ToJsonUtilE(Permission, each_Permission); output["Permission"] = each_Permission;
                Json::Value each_SharedGroupId; ToJsonUtilS(SharedGroupId, each_SharedGroupId); output["SharedGroupId"] = each_SharedGroupId;
                return output;
            }
        };

        struct UpdateSharedGroupDataResult : public PlayFabResultCommon
        {

            UpdateSharedGroupDataResult() :
                PlayFabResultCommon()
            {}

            UpdateSharedGroupDataResult(const UpdateSharedGroupDataResult&) :
                PlayFabResultCommon()
            {}

            ~UpdateSharedGroupDataResult() = default;

            void FromJson(const Json::Value&) override
            {
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                return output;
            }
        };

        struct UpdateUserDataRequest : public PlayFabRequestCommon
        {
            std::map<std::string, std::string> CustomTags;
            std::map<std::string, std::string> Data;
            std::list<std::string> KeysToRemove;
            Boxed<UserDataPermission> Permission;

            UpdateUserDataRequest() :
                PlayFabRequestCommon(),
                CustomTags(),
                Data(),
                KeysToRemove(),
                Permission()
            {}

            UpdateUserDataRequest(const UpdateUserDataRequest& src) :
                PlayFabRequestCommon(),
                CustomTags(src.CustomTags),
                Data(src.Data),
                KeysToRemove(src.KeysToRemove),
                Permission(src.Permission)
            {}

            ~UpdateUserDataRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["CustomTags"], CustomTags);
                FromJsonUtilS(input["Data"], Data);
                FromJsonUtilS(input["KeysToRemove"], KeysToRemove);
                FromJsonUtilE(input["Permission"], Permission);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_CustomTags; ToJsonUtilS(CustomTags, each_CustomTags); output["CustomTags"] = each_CustomTags;
                Json::Value each_Data; ToJsonUtilS(Data, each_Data); output["Data"] = each_Data;
                Json::Value each_KeysToRemove; ToJsonUtilS(KeysToRemove, each_KeysToRemove); output["KeysToRemove"] = each_KeysToRemove;
                Json::Value each_Permission; ToJsonUtilE(Permission, each_Permission); output["Permission"] = each_Permission;
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

        struct UpdateUserTitleDisplayNameRequest : public PlayFabRequestCommon
        {
            std::map<std::string, std::string> CustomTags;
            std::string DisplayName;

            UpdateUserTitleDisplayNameRequest() :
                PlayFabRequestCommon(),
                CustomTags(),
                DisplayName()
            {}

            UpdateUserTitleDisplayNameRequest(const UpdateUserTitleDisplayNameRequest& src) :
                PlayFabRequestCommon(),
                CustomTags(src.CustomTags),
                DisplayName(src.DisplayName)
            {}

            ~UpdateUserTitleDisplayNameRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["CustomTags"], CustomTags);
                FromJsonUtilS(input["DisplayName"], DisplayName);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_CustomTags; ToJsonUtilS(CustomTags, each_CustomTags); output["CustomTags"] = each_CustomTags;
                Json::Value each_DisplayName; ToJsonUtilS(DisplayName, each_DisplayName); output["DisplayName"] = each_DisplayName;
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

        struct ValidateAmazonReceiptRequest : public PlayFabRequestCommon
        {
            std::string CatalogVersion;
            std::string CurrencyCode;
            std::map<std::string, std::string> CustomTags;
            Int32 PurchasePrice;
            std::string ReceiptId;
            std::string UserId;

            ValidateAmazonReceiptRequest() :
                PlayFabRequestCommon(),
                CatalogVersion(),
                CurrencyCode(),
                CustomTags(),
                PurchasePrice(),
                ReceiptId(),
                UserId()
            {}

            ValidateAmazonReceiptRequest(const ValidateAmazonReceiptRequest& src) :
                PlayFabRequestCommon(),
                CatalogVersion(src.CatalogVersion),
                CurrencyCode(src.CurrencyCode),
                CustomTags(src.CustomTags),
                PurchasePrice(src.PurchasePrice),
                ReceiptId(src.ReceiptId),
                UserId(src.UserId)
            {}

            ~ValidateAmazonReceiptRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["CatalogVersion"], CatalogVersion);
                FromJsonUtilS(input["CurrencyCode"], CurrencyCode);
                FromJsonUtilS(input["CustomTags"], CustomTags);
                FromJsonUtilP(input["PurchasePrice"], PurchasePrice);
                FromJsonUtilS(input["ReceiptId"], ReceiptId);
                FromJsonUtilS(input["UserId"], UserId);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_CatalogVersion; ToJsonUtilS(CatalogVersion, each_CatalogVersion); output["CatalogVersion"] = each_CatalogVersion;
                Json::Value each_CurrencyCode; ToJsonUtilS(CurrencyCode, each_CurrencyCode); output["CurrencyCode"] = each_CurrencyCode;
                Json::Value each_CustomTags; ToJsonUtilS(CustomTags, each_CustomTags); output["CustomTags"] = each_CustomTags;
                Json::Value each_PurchasePrice; ToJsonUtilP(PurchasePrice, each_PurchasePrice); output["PurchasePrice"] = each_PurchasePrice;
                Json::Value each_ReceiptId; ToJsonUtilS(ReceiptId, each_ReceiptId); output["ReceiptId"] = each_ReceiptId;
                Json::Value each_UserId; ToJsonUtilS(UserId, each_UserId); output["UserId"] = each_UserId;
                return output;
            }
        };

        struct ValidateAmazonReceiptResult : public PlayFabResultCommon
        {
            std::list<PurchaseReceiptFulfillment> Fulfillments;

            ValidateAmazonReceiptResult() :
                PlayFabResultCommon(),
                Fulfillments()
            {}

            ValidateAmazonReceiptResult(const ValidateAmazonReceiptResult& src) :
                PlayFabResultCommon(),
                Fulfillments(src.Fulfillments)
            {}

            ~ValidateAmazonReceiptResult() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilO(input["Fulfillments"], Fulfillments);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_Fulfillments; ToJsonUtilO(Fulfillments, each_Fulfillments); output["Fulfillments"] = each_Fulfillments;
                return output;
            }
        };

        struct ValidateGooglePlayPurchaseRequest : public PlayFabRequestCommon
        {
            std::string CatalogVersion;
            std::string CurrencyCode;
            std::map<std::string, std::string> CustomTags;
            Boxed<Uint32> PurchasePrice;
            std::string ReceiptJson;
            std::string Signature;

            ValidateGooglePlayPurchaseRequest() :
                PlayFabRequestCommon(),
                CatalogVersion(),
                CurrencyCode(),
                CustomTags(),
                PurchasePrice(),
                ReceiptJson(),
                Signature()
            {}

            ValidateGooglePlayPurchaseRequest(const ValidateGooglePlayPurchaseRequest& src) :
                PlayFabRequestCommon(),
                CatalogVersion(src.CatalogVersion),
                CurrencyCode(src.CurrencyCode),
                CustomTags(src.CustomTags),
                PurchasePrice(src.PurchasePrice),
                ReceiptJson(src.ReceiptJson),
                Signature(src.Signature)
            {}

            ~ValidateGooglePlayPurchaseRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["CatalogVersion"], CatalogVersion);
                FromJsonUtilS(input["CurrencyCode"], CurrencyCode);
                FromJsonUtilS(input["CustomTags"], CustomTags);
                FromJsonUtilP(input["PurchasePrice"], PurchasePrice);
                FromJsonUtilS(input["ReceiptJson"], ReceiptJson);
                FromJsonUtilS(input["Signature"], Signature);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_CatalogVersion; ToJsonUtilS(CatalogVersion, each_CatalogVersion); output["CatalogVersion"] = each_CatalogVersion;
                Json::Value each_CurrencyCode; ToJsonUtilS(CurrencyCode, each_CurrencyCode); output["CurrencyCode"] = each_CurrencyCode;
                Json::Value each_CustomTags; ToJsonUtilS(CustomTags, each_CustomTags); output["CustomTags"] = each_CustomTags;
                Json::Value each_PurchasePrice; ToJsonUtilP(PurchasePrice, each_PurchasePrice); output["PurchasePrice"] = each_PurchasePrice;
                Json::Value each_ReceiptJson; ToJsonUtilS(ReceiptJson, each_ReceiptJson); output["ReceiptJson"] = each_ReceiptJson;
                Json::Value each_Signature; ToJsonUtilS(Signature, each_Signature); output["Signature"] = each_Signature;
                return output;
            }
        };

        struct ValidateGooglePlayPurchaseResult : public PlayFabResultCommon
        {
            std::list<PurchaseReceiptFulfillment> Fulfillments;

            ValidateGooglePlayPurchaseResult() :
                PlayFabResultCommon(),
                Fulfillments()
            {}

            ValidateGooglePlayPurchaseResult(const ValidateGooglePlayPurchaseResult& src) :
                PlayFabResultCommon(),
                Fulfillments(src.Fulfillments)
            {}

            ~ValidateGooglePlayPurchaseResult() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilO(input["Fulfillments"], Fulfillments);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_Fulfillments; ToJsonUtilO(Fulfillments, each_Fulfillments); output["Fulfillments"] = each_Fulfillments;
                return output;
            }
        };

        struct ValidateIOSReceiptRequest : public PlayFabRequestCommon
        {
            std::string CatalogVersion;
            std::string CurrencyCode;
            std::map<std::string, std::string> CustomTags;
            Int32 PurchasePrice;
            std::string ReceiptData;

            ValidateIOSReceiptRequest() :
                PlayFabRequestCommon(),
                CatalogVersion(),
                CurrencyCode(),
                CustomTags(),
                PurchasePrice(),
                ReceiptData()
            {}

            ValidateIOSReceiptRequest(const ValidateIOSReceiptRequest& src) :
                PlayFabRequestCommon(),
                CatalogVersion(src.CatalogVersion),
                CurrencyCode(src.CurrencyCode),
                CustomTags(src.CustomTags),
                PurchasePrice(src.PurchasePrice),
                ReceiptData(src.ReceiptData)
            {}

            ~ValidateIOSReceiptRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["CatalogVersion"], CatalogVersion);
                FromJsonUtilS(input["CurrencyCode"], CurrencyCode);
                FromJsonUtilS(input["CustomTags"], CustomTags);
                FromJsonUtilP(input["PurchasePrice"], PurchasePrice);
                FromJsonUtilS(input["ReceiptData"], ReceiptData);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_CatalogVersion; ToJsonUtilS(CatalogVersion, each_CatalogVersion); output["CatalogVersion"] = each_CatalogVersion;
                Json::Value each_CurrencyCode; ToJsonUtilS(CurrencyCode, each_CurrencyCode); output["CurrencyCode"] = each_CurrencyCode;
                Json::Value each_CustomTags; ToJsonUtilS(CustomTags, each_CustomTags); output["CustomTags"] = each_CustomTags;
                Json::Value each_PurchasePrice; ToJsonUtilP(PurchasePrice, each_PurchasePrice); output["PurchasePrice"] = each_PurchasePrice;
                Json::Value each_ReceiptData; ToJsonUtilS(ReceiptData, each_ReceiptData); output["ReceiptData"] = each_ReceiptData;
                return output;
            }
        };

        struct ValidateIOSReceiptResult : public PlayFabResultCommon
        {
            std::list<PurchaseReceiptFulfillment> Fulfillments;

            ValidateIOSReceiptResult() :
                PlayFabResultCommon(),
                Fulfillments()
            {}

            ValidateIOSReceiptResult(const ValidateIOSReceiptResult& src) :
                PlayFabResultCommon(),
                Fulfillments(src.Fulfillments)
            {}

            ~ValidateIOSReceiptResult() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilO(input["Fulfillments"], Fulfillments);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_Fulfillments; ToJsonUtilO(Fulfillments, each_Fulfillments); output["Fulfillments"] = each_Fulfillments;
                return output;
            }
        };

        struct ValidateWindowsReceiptRequest : public PlayFabRequestCommon
        {
            std::string CatalogVersion;
            std::string CurrencyCode;
            std::map<std::string, std::string> CustomTags;
            Uint32 PurchasePrice;
            std::string Receipt;

            ValidateWindowsReceiptRequest() :
                PlayFabRequestCommon(),
                CatalogVersion(),
                CurrencyCode(),
                CustomTags(),
                PurchasePrice(),
                Receipt()
            {}

            ValidateWindowsReceiptRequest(const ValidateWindowsReceiptRequest& src) :
                PlayFabRequestCommon(),
                CatalogVersion(src.CatalogVersion),
                CurrencyCode(src.CurrencyCode),
                CustomTags(src.CustomTags),
                PurchasePrice(src.PurchasePrice),
                Receipt(src.Receipt)
            {}

            ~ValidateWindowsReceiptRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["CatalogVersion"], CatalogVersion);
                FromJsonUtilS(input["CurrencyCode"], CurrencyCode);
                FromJsonUtilS(input["CustomTags"], CustomTags);
                FromJsonUtilP(input["PurchasePrice"], PurchasePrice);
                FromJsonUtilS(input["Receipt"], Receipt);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_CatalogVersion; ToJsonUtilS(CatalogVersion, each_CatalogVersion); output["CatalogVersion"] = each_CatalogVersion;
                Json::Value each_CurrencyCode; ToJsonUtilS(CurrencyCode, each_CurrencyCode); output["CurrencyCode"] = each_CurrencyCode;
                Json::Value each_CustomTags; ToJsonUtilS(CustomTags, each_CustomTags); output["CustomTags"] = each_CustomTags;
                Json::Value each_PurchasePrice; ToJsonUtilP(PurchasePrice, each_PurchasePrice); output["PurchasePrice"] = each_PurchasePrice;
                Json::Value each_Receipt; ToJsonUtilS(Receipt, each_Receipt); output["Receipt"] = each_Receipt;
                return output;
            }
        };

        struct ValidateWindowsReceiptResult : public PlayFabResultCommon
        {
            std::list<PurchaseReceiptFulfillment> Fulfillments;

            ValidateWindowsReceiptResult() :
                PlayFabResultCommon(),
                Fulfillments()
            {}

            ValidateWindowsReceiptResult(const ValidateWindowsReceiptResult& src) :
                PlayFabResultCommon(),
                Fulfillments(src.Fulfillments)
            {}

            ~ValidateWindowsReceiptResult() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilO(input["Fulfillments"], Fulfillments);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_Fulfillments; ToJsonUtilO(Fulfillments, each_Fulfillments); output["Fulfillments"] = each_Fulfillments;
                return output;
            }
        };

        struct WriteClientCharacterEventRequest : public PlayFabRequestCommon
        {
            Json::Value Body; // Not truly arbitrary. See documentation for restrictions on format
            std::string CharacterId;
            std::map<std::string, std::string> CustomTags;
            std::string EventName;
            Boxed<time_t> Timestamp;

            WriteClientCharacterEventRequest() :
                PlayFabRequestCommon(),
                Body(),
                CharacterId(),
                CustomTags(),
                EventName(),
                Timestamp()
            {}

            WriteClientCharacterEventRequest(const WriteClientCharacterEventRequest& src) :
                PlayFabRequestCommon(),
                Body(src.Body),
                CharacterId(src.CharacterId),
                CustomTags(src.CustomTags),
                EventName(src.EventName),
                Timestamp(src.Timestamp)
            {}

            ~WriteClientCharacterEventRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                Body = input["Body"];
                FromJsonUtilS(input["CharacterId"], CharacterId);
                FromJsonUtilS(input["CustomTags"], CustomTags);
                FromJsonUtilS(input["EventName"], EventName);
                FromJsonUtilT(input["Timestamp"], Timestamp);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                output["Body"] = Body;
                Json::Value each_CharacterId; ToJsonUtilS(CharacterId, each_CharacterId); output["CharacterId"] = each_CharacterId;
                Json::Value each_CustomTags; ToJsonUtilS(CustomTags, each_CustomTags); output["CustomTags"] = each_CustomTags;
                Json::Value each_EventName; ToJsonUtilS(EventName, each_EventName); output["EventName"] = each_EventName;
                Json::Value each_Timestamp; ToJsonUtilT(Timestamp, each_Timestamp); output["Timestamp"] = each_Timestamp;
                return output;
            }
        };

        struct WriteClientPlayerEventRequest : public PlayFabRequestCommon
        {
            Json::Value Body; // Not truly arbitrary. See documentation for restrictions on format
            std::map<std::string, std::string> CustomTags;
            std::string EventName;
            Boxed<time_t> Timestamp;

            WriteClientPlayerEventRequest() :
                PlayFabRequestCommon(),
                Body(),
                CustomTags(),
                EventName(),
                Timestamp()
            {}

            WriteClientPlayerEventRequest(const WriteClientPlayerEventRequest& src) :
                PlayFabRequestCommon(),
                Body(src.Body),
                CustomTags(src.CustomTags),
                EventName(src.EventName),
                Timestamp(src.Timestamp)
            {}

            ~WriteClientPlayerEventRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                Body = input["Body"];
                FromJsonUtilS(input["CustomTags"], CustomTags);
                FromJsonUtilS(input["EventName"], EventName);
                FromJsonUtilT(input["Timestamp"], Timestamp);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                output["Body"] = Body;
                Json::Value each_CustomTags; ToJsonUtilS(CustomTags, each_CustomTags); output["CustomTags"] = each_CustomTags;
                Json::Value each_EventName; ToJsonUtilS(EventName, each_EventName); output["EventName"] = each_EventName;
                Json::Value each_Timestamp; ToJsonUtilT(Timestamp, each_Timestamp); output["Timestamp"] = each_Timestamp;
                return output;
            }
        };

        struct WriteEventResponse : public PlayFabResultCommon
        {
            std::string EventId;

            WriteEventResponse() :
                PlayFabResultCommon(),
                EventId()
            {}

            WriteEventResponse(const WriteEventResponse& src) :
                PlayFabResultCommon(),
                EventId(src.EventId)
            {}

            ~WriteEventResponse() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["EventId"], EventId);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_EventId; ToJsonUtilS(EventId, each_EventId); output["EventId"] = each_EventId;
                return output;
            }
        };

        struct WriteTitleEventRequest : public PlayFabRequestCommon
        {
            Json::Value Body; // Not truly arbitrary. See documentation for restrictions on format
            std::map<std::string, std::string> CustomTags;
            std::string EventName;
            Boxed<time_t> Timestamp;

            WriteTitleEventRequest() :
                PlayFabRequestCommon(),
                Body(),
                CustomTags(),
                EventName(),
                Timestamp()
            {}

            WriteTitleEventRequest(const WriteTitleEventRequest& src) :
                PlayFabRequestCommon(),
                Body(src.Body),
                CustomTags(src.CustomTags),
                EventName(src.EventName),
                Timestamp(src.Timestamp)
            {}

            ~WriteTitleEventRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                Body = input["Body"];
                FromJsonUtilS(input["CustomTags"], CustomTags);
                FromJsonUtilS(input["EventName"], EventName);
                FromJsonUtilT(input["Timestamp"], Timestamp);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                output["Body"] = Body;
                Json::Value each_CustomTags; ToJsonUtilS(CustomTags, each_CustomTags); output["CustomTags"] = each_CustomTags;
                Json::Value each_EventName; ToJsonUtilS(EventName, each_EventName); output["EventName"] = each_EventName;
                Json::Value each_Timestamp; ToJsonUtilT(Timestamp, each_Timestamp); output["Timestamp"] = each_Timestamp;
                return output;
            }
        };

    }
}

#endif
