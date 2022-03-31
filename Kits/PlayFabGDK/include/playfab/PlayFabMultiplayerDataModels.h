#pragma once

#if !defined(DISABLE_PLAYFABENTITY_API)

#include <playfab/PlayFabBaseModel.h>
#include <playfab/PlayFabJsonHeaders.h>

namespace PlayFab
{
    namespace MultiplayerModels
    {
        // Multiplayer Enums
        enum class AzureRegion
        {
            AzureRegionAustraliaEast,
            AzureRegionAustraliaSoutheast,
            AzureRegionBrazilSouth,
            AzureRegionCentralUs,
            AzureRegionEastAsia,
            AzureRegionEastUs,
            AzureRegionEastUs2,
            AzureRegionJapanEast,
            AzureRegionJapanWest,
            AzureRegionNorthCentralUs,
            AzureRegionNorthEurope,
            AzureRegionSouthCentralUs,
            AzureRegionSoutheastAsia,
            AzureRegionWestEurope,
            AzureRegionWestUs,
            AzureRegionSouthAfricaNorth,
            AzureRegionWestCentralUs,
            AzureRegionKoreaCentral,
            AzureRegionFranceCentral,
            AzureRegionWestUs2,
            AzureRegionCentralIndia,
            AzureRegionUaeNorth,
            AzureRegionUkSouth
        };

        inline void ToJsonEnum(const AzureRegion input, Json::Value& output)
        {
            if (input == AzureRegion::AzureRegionAustraliaEast)
            {
                output = Json::Value("AustraliaEast");
                return;
            }
            if (input == AzureRegion::AzureRegionAustraliaSoutheast)
            {
                output = Json::Value("AustraliaSoutheast");
                return;
            }
            if (input == AzureRegion::AzureRegionBrazilSouth)
            {
                output = Json::Value("BrazilSouth");
                return;
            }
            if (input == AzureRegion::AzureRegionCentralUs)
            {
                output = Json::Value("CentralUs");
                return;
            }
            if (input == AzureRegion::AzureRegionEastAsia)
            {
                output = Json::Value("EastAsia");
                return;
            }
            if (input == AzureRegion::AzureRegionEastUs)
            {
                output = Json::Value("EastUs");
                return;
            }
            if (input == AzureRegion::AzureRegionEastUs2)
            {
                output = Json::Value("EastUs2");
                return;
            }
            if (input == AzureRegion::AzureRegionJapanEast)
            {
                output = Json::Value("JapanEast");
                return;
            }
            if (input == AzureRegion::AzureRegionJapanWest)
            {
                output = Json::Value("JapanWest");
                return;
            }
            if (input == AzureRegion::AzureRegionNorthCentralUs)
            {
                output = Json::Value("NorthCentralUs");
                return;
            }
            if (input == AzureRegion::AzureRegionNorthEurope)
            {
                output = Json::Value("NorthEurope");
                return;
            }
            if (input == AzureRegion::AzureRegionSouthCentralUs)
            {
                output = Json::Value("SouthCentralUs");
                return;
            }
            if (input == AzureRegion::AzureRegionSoutheastAsia)
            {
                output = Json::Value("SoutheastAsia");
                return;
            }
            if (input == AzureRegion::AzureRegionWestEurope)
            {
                output = Json::Value("WestEurope");
                return;
            }
            if (input == AzureRegion::AzureRegionWestUs)
            {
                output = Json::Value("WestUs");
                return;
            }
            if (input == AzureRegion::AzureRegionSouthAfricaNorth)
            {
                output = Json::Value("SouthAfricaNorth");
                return;
            }
            if (input == AzureRegion::AzureRegionWestCentralUs)
            {
                output = Json::Value("WestCentralUs");
                return;
            }
            if (input == AzureRegion::AzureRegionKoreaCentral)
            {
                output = Json::Value("KoreaCentral");
                return;
            }
            if (input == AzureRegion::AzureRegionFranceCentral)
            {
                output = Json::Value("FranceCentral");
                return;
            }
            if (input == AzureRegion::AzureRegionWestUs2)
            {
                output = Json::Value("WestUs2");
                return;
            }
            if (input == AzureRegion::AzureRegionCentralIndia)
            {
                output = Json::Value("CentralIndia");
                return;
            }
            if (input == AzureRegion::AzureRegionUaeNorth)
            {
                output = Json::Value("UaeNorth");
                return;
            }
            if (input == AzureRegion::AzureRegionUkSouth)
            {
                output = Json::Value("UkSouth");
                return;
            }
        }
        inline void FromJsonEnum(const Json::Value& input, AzureRegion& output)
        {
            if (!input.isString())
            {
                return;
            }
            const std::string& inputStr = input.asString();
            if (inputStr == "AustraliaEast")
            {
                output = AzureRegion::AzureRegionAustraliaEast;
                return;
            }
            if (inputStr == "AustraliaSoutheast")
            {
                output = AzureRegion::AzureRegionAustraliaSoutheast;
                return;
            }
            if (inputStr == "BrazilSouth")
            {
                output = AzureRegion::AzureRegionBrazilSouth;
                return;
            }
            if (inputStr == "CentralUs")
            {
                output = AzureRegion::AzureRegionCentralUs;
                return;
            }
            if (inputStr == "EastAsia")
            {
                output = AzureRegion::AzureRegionEastAsia;
                return;
            }
            if (inputStr == "EastUs")
            {
                output = AzureRegion::AzureRegionEastUs;
                return;
            }
            if (inputStr == "EastUs2")
            {
                output = AzureRegion::AzureRegionEastUs2;
                return;
            }
            if (inputStr == "JapanEast")
            {
                output = AzureRegion::AzureRegionJapanEast;
                return;
            }
            if (inputStr == "JapanWest")
            {
                output = AzureRegion::AzureRegionJapanWest;
                return;
            }
            if (inputStr == "NorthCentralUs")
            {
                output = AzureRegion::AzureRegionNorthCentralUs;
                return;
            }
            if (inputStr == "NorthEurope")
            {
                output = AzureRegion::AzureRegionNorthEurope;
                return;
            }
            if (inputStr == "SouthCentralUs")
            {
                output = AzureRegion::AzureRegionSouthCentralUs;
                return;
            }
            if (inputStr == "SoutheastAsia")
            {
                output = AzureRegion::AzureRegionSoutheastAsia;
                return;
            }
            if (inputStr == "WestEurope")
            {
                output = AzureRegion::AzureRegionWestEurope;
                return;
            }
            if (inputStr == "WestUs")
            {
                output = AzureRegion::AzureRegionWestUs;
                return;
            }
            if (inputStr == "SouthAfricaNorth")
            {
                output = AzureRegion::AzureRegionSouthAfricaNorth;
                return;
            }
            if (inputStr == "WestCentralUs")
            {
                output = AzureRegion::AzureRegionWestCentralUs;
                return;
            }
            if (inputStr == "KoreaCentral")
            {
                output = AzureRegion::AzureRegionKoreaCentral;
                return;
            }
            if (inputStr == "FranceCentral")
            {
                output = AzureRegion::AzureRegionFranceCentral;
                return;
            }
            if (inputStr == "WestUs2")
            {
                output = AzureRegion::AzureRegionWestUs2;
                return;
            }
            if (inputStr == "CentralIndia")
            {
                output = AzureRegion::AzureRegionCentralIndia;
                return;
            }
            if (inputStr == "UaeNorth")
            {
                output = AzureRegion::AzureRegionUaeNorth;
                return;
            }
            if (inputStr == "UkSouth")
            {
                output = AzureRegion::AzureRegionUkSouth;
                return;
            }
        }

        enum class AzureVmFamily
        {
            AzureVmFamilyA,
            AzureVmFamilyAv2,
            AzureVmFamilyDv2,
            AzureVmFamilyDv3,
            AzureVmFamilyF,
            AzureVmFamilyFsv2,
            AzureVmFamilyDasv4,
            AzureVmFamilyDav4,
            AzureVmFamilyEav4,
            AzureVmFamilyEasv4,
            AzureVmFamilyEv4,
            AzureVmFamilyEsv4,
            AzureVmFamilyDsv3,
            AzureVmFamilyDsv2,
            AzureVmFamilyNCasT4_v3,
            AzureVmFamilyDdv4,
            AzureVmFamilyDdsv4,
            AzureVmFamilyHBv3
        };

        inline void ToJsonEnum(const AzureVmFamily input, Json::Value& output)
        {
            if (input == AzureVmFamily::AzureVmFamilyA)
            {
                output = Json::Value("A");
                return;
            }
            if (input == AzureVmFamily::AzureVmFamilyAv2)
            {
                output = Json::Value("Av2");
                return;
            }
            if (input == AzureVmFamily::AzureVmFamilyDv2)
            {
                output = Json::Value("Dv2");
                return;
            }
            if (input == AzureVmFamily::AzureVmFamilyDv3)
            {
                output = Json::Value("Dv3");
                return;
            }
            if (input == AzureVmFamily::AzureVmFamilyF)
            {
                output = Json::Value("F");
                return;
            }
            if (input == AzureVmFamily::AzureVmFamilyFsv2)
            {
                output = Json::Value("Fsv2");
                return;
            }
            if (input == AzureVmFamily::AzureVmFamilyDasv4)
            {
                output = Json::Value("Dasv4");
                return;
            }
            if (input == AzureVmFamily::AzureVmFamilyDav4)
            {
                output = Json::Value("Dav4");
                return;
            }
            if (input == AzureVmFamily::AzureVmFamilyEav4)
            {
                output = Json::Value("Eav4");
                return;
            }
            if (input == AzureVmFamily::AzureVmFamilyEasv4)
            {
                output = Json::Value("Easv4");
                return;
            }
            if (input == AzureVmFamily::AzureVmFamilyEv4)
            {
                output = Json::Value("Ev4");
                return;
            }
            if (input == AzureVmFamily::AzureVmFamilyEsv4)
            {
                output = Json::Value("Esv4");
                return;
            }
            if (input == AzureVmFamily::AzureVmFamilyDsv3)
            {
                output = Json::Value("Dsv3");
                return;
            }
            if (input == AzureVmFamily::AzureVmFamilyDsv2)
            {
                output = Json::Value("Dsv2");
                return;
            }
            if (input == AzureVmFamily::AzureVmFamilyNCasT4_v3)
            {
                output = Json::Value("NCasT4_v3");
                return;
            }
            if (input == AzureVmFamily::AzureVmFamilyDdv4)
            {
                output = Json::Value("Ddv4");
                return;
            }
            if (input == AzureVmFamily::AzureVmFamilyDdsv4)
            {
                output = Json::Value("Ddsv4");
                return;
            }
            if (input == AzureVmFamily::AzureVmFamilyHBv3)
            {
                output = Json::Value("HBv3");
                return;
            }
        }
        inline void FromJsonEnum(const Json::Value& input, AzureVmFamily& output)
        {
            if (!input.isString())
            {
                return;
            }
            const std::string& inputStr = input.asString();
            if (inputStr == "A")
            {
                output = AzureVmFamily::AzureVmFamilyA;
                return;
            }
            if (inputStr == "Av2")
            {
                output = AzureVmFamily::AzureVmFamilyAv2;
                return;
            }
            if (inputStr == "Dv2")
            {
                output = AzureVmFamily::AzureVmFamilyDv2;
                return;
            }
            if (inputStr == "Dv3")
            {
                output = AzureVmFamily::AzureVmFamilyDv3;
                return;
            }
            if (inputStr == "F")
            {
                output = AzureVmFamily::AzureVmFamilyF;
                return;
            }
            if (inputStr == "Fsv2")
            {
                output = AzureVmFamily::AzureVmFamilyFsv2;
                return;
            }
            if (inputStr == "Dasv4")
            {
                output = AzureVmFamily::AzureVmFamilyDasv4;
                return;
            }
            if (inputStr == "Dav4")
            {
                output = AzureVmFamily::AzureVmFamilyDav4;
                return;
            }
            if (inputStr == "Eav4")
            {
                output = AzureVmFamily::AzureVmFamilyEav4;
                return;
            }
            if (inputStr == "Easv4")
            {
                output = AzureVmFamily::AzureVmFamilyEasv4;
                return;
            }
            if (inputStr == "Ev4")
            {
                output = AzureVmFamily::AzureVmFamilyEv4;
                return;
            }
            if (inputStr == "Esv4")
            {
                output = AzureVmFamily::AzureVmFamilyEsv4;
                return;
            }
            if (inputStr == "Dsv3")
            {
                output = AzureVmFamily::AzureVmFamilyDsv3;
                return;
            }
            if (inputStr == "Dsv2")
            {
                output = AzureVmFamily::AzureVmFamilyDsv2;
                return;
            }
            if (inputStr == "NCasT4_v3")
            {
                output = AzureVmFamily::AzureVmFamilyNCasT4_v3;
                return;
            }
            if (inputStr == "Ddv4")
            {
                output = AzureVmFamily::AzureVmFamilyDdv4;
                return;
            }
            if (inputStr == "Ddsv4")
            {
                output = AzureVmFamily::AzureVmFamilyDdsv4;
                return;
            }
            if (inputStr == "HBv3")
            {
                output = AzureVmFamily::AzureVmFamilyHBv3;
                return;
            }
        }

        enum class AzureVmSize
        {
            AzureVmSizeStandard_A1,
            AzureVmSizeStandard_A2,
            AzureVmSizeStandard_A3,
            AzureVmSizeStandard_A4,
            AzureVmSizeStandard_A1_v2,
            AzureVmSizeStandard_A2_v2,
            AzureVmSizeStandard_A4_v2,
            AzureVmSizeStandard_A8_v2,
            AzureVmSizeStandard_D1_v2,
            AzureVmSizeStandard_D2_v2,
            AzureVmSizeStandard_D3_v2,
            AzureVmSizeStandard_D4_v2,
            AzureVmSizeStandard_D5_v2,
            AzureVmSizeStandard_D2_v3,
            AzureVmSizeStandard_D4_v3,
            AzureVmSizeStandard_D8_v3,
            AzureVmSizeStandard_D16_v3,
            AzureVmSizeStandard_F1,
            AzureVmSizeStandard_F2,
            AzureVmSizeStandard_F4,
            AzureVmSizeStandard_F8,
            AzureVmSizeStandard_F16,
            AzureVmSizeStandard_F2s_v2,
            AzureVmSizeStandard_F4s_v2,
            AzureVmSizeStandard_F8s_v2,
            AzureVmSizeStandard_F16s_v2,
            AzureVmSizeStandard_D2as_v4,
            AzureVmSizeStandard_D4as_v4,
            AzureVmSizeStandard_D8as_v4,
            AzureVmSizeStandard_D16as_v4,
            AzureVmSizeStandard_D2a_v4,
            AzureVmSizeStandard_D4a_v4,
            AzureVmSizeStandard_D8a_v4,
            AzureVmSizeStandard_D16a_v4,
            AzureVmSizeStandard_E2a_v4,
            AzureVmSizeStandard_E4a_v4,
            AzureVmSizeStandard_E8a_v4,
            AzureVmSizeStandard_E16a_v4,
            AzureVmSizeStandard_E2as_v4,
            AzureVmSizeStandard_E4as_v4,
            AzureVmSizeStandard_E8as_v4,
            AzureVmSizeStandard_E16as_v4,
            AzureVmSizeStandard_D2s_v3,
            AzureVmSizeStandard_D4s_v3,
            AzureVmSizeStandard_D8s_v3,
            AzureVmSizeStandard_D16s_v3,
            AzureVmSizeStandard_DS1_v2,
            AzureVmSizeStandard_DS2_v2,
            AzureVmSizeStandard_DS3_v2,
            AzureVmSizeStandard_DS4_v2,
            AzureVmSizeStandard_DS5_v2,
            AzureVmSizeStandard_NC4as_T4_v3,
            AzureVmSizeStandard_D2d_v4,
            AzureVmSizeStandard_D4d_v4,
            AzureVmSizeStandard_D8d_v4,
            AzureVmSizeStandard_D16d_v4,
            AzureVmSizeStandard_D2ds_v4,
            AzureVmSizeStandard_D4ds_v4,
            AzureVmSizeStandard_D8ds_v4,
            AzureVmSizeStandard_D16ds_v4,
            AzureVmSizeStandard_HB120_16rs_v3,
            AzureVmSizeStandard_HB120_32rs_v3,
            AzureVmSizeStandard_HB120_64rs_v3,
            AzureVmSizeStandard_HB120_96rs_v3,
            AzureVmSizeStandard_HB120rs_v3
        };

        inline void ToJsonEnum(const AzureVmSize input, Json::Value& output)
        {
            if (input == AzureVmSize::AzureVmSizeStandard_A1)
            {
                output = Json::Value("Standard_A1");
                return;
            }
            if (input == AzureVmSize::AzureVmSizeStandard_A2)
            {
                output = Json::Value("Standard_A2");
                return;
            }
            if (input == AzureVmSize::AzureVmSizeStandard_A3)
            {
                output = Json::Value("Standard_A3");
                return;
            }
            if (input == AzureVmSize::AzureVmSizeStandard_A4)
            {
                output = Json::Value("Standard_A4");
                return;
            }
            if (input == AzureVmSize::AzureVmSizeStandard_A1_v2)
            {
                output = Json::Value("Standard_A1_v2");
                return;
            }
            if (input == AzureVmSize::AzureVmSizeStandard_A2_v2)
            {
                output = Json::Value("Standard_A2_v2");
                return;
            }
            if (input == AzureVmSize::AzureVmSizeStandard_A4_v2)
            {
                output = Json::Value("Standard_A4_v2");
                return;
            }
            if (input == AzureVmSize::AzureVmSizeStandard_A8_v2)
            {
                output = Json::Value("Standard_A8_v2");
                return;
            }
            if (input == AzureVmSize::AzureVmSizeStandard_D1_v2)
            {
                output = Json::Value("Standard_D1_v2");
                return;
            }
            if (input == AzureVmSize::AzureVmSizeStandard_D2_v2)
            {
                output = Json::Value("Standard_D2_v2");
                return;
            }
            if (input == AzureVmSize::AzureVmSizeStandard_D3_v2)
            {
                output = Json::Value("Standard_D3_v2");
                return;
            }
            if (input == AzureVmSize::AzureVmSizeStandard_D4_v2)
            {
                output = Json::Value("Standard_D4_v2");
                return;
            }
            if (input == AzureVmSize::AzureVmSizeStandard_D5_v2)
            {
                output = Json::Value("Standard_D5_v2");
                return;
            }
            if (input == AzureVmSize::AzureVmSizeStandard_D2_v3)
            {
                output = Json::Value("Standard_D2_v3");
                return;
            }
            if (input == AzureVmSize::AzureVmSizeStandard_D4_v3)
            {
                output = Json::Value("Standard_D4_v3");
                return;
            }
            if (input == AzureVmSize::AzureVmSizeStandard_D8_v3)
            {
                output = Json::Value("Standard_D8_v3");
                return;
            }
            if (input == AzureVmSize::AzureVmSizeStandard_D16_v3)
            {
                output = Json::Value("Standard_D16_v3");
                return;
            }
            if (input == AzureVmSize::AzureVmSizeStandard_F1)
            {
                output = Json::Value("Standard_F1");
                return;
            }
            if (input == AzureVmSize::AzureVmSizeStandard_F2)
            {
                output = Json::Value("Standard_F2");
                return;
            }
            if (input == AzureVmSize::AzureVmSizeStandard_F4)
            {
                output = Json::Value("Standard_F4");
                return;
            }
            if (input == AzureVmSize::AzureVmSizeStandard_F8)
            {
                output = Json::Value("Standard_F8");
                return;
            }
            if (input == AzureVmSize::AzureVmSizeStandard_F16)
            {
                output = Json::Value("Standard_F16");
                return;
            }
            if (input == AzureVmSize::AzureVmSizeStandard_F2s_v2)
            {
                output = Json::Value("Standard_F2s_v2");
                return;
            }
            if (input == AzureVmSize::AzureVmSizeStandard_F4s_v2)
            {
                output = Json::Value("Standard_F4s_v2");
                return;
            }
            if (input == AzureVmSize::AzureVmSizeStandard_F8s_v2)
            {
                output = Json::Value("Standard_F8s_v2");
                return;
            }
            if (input == AzureVmSize::AzureVmSizeStandard_F16s_v2)
            {
                output = Json::Value("Standard_F16s_v2");
                return;
            }
            if (input == AzureVmSize::AzureVmSizeStandard_D2as_v4)
            {
                output = Json::Value("Standard_D2as_v4");
                return;
            }
            if (input == AzureVmSize::AzureVmSizeStandard_D4as_v4)
            {
                output = Json::Value("Standard_D4as_v4");
                return;
            }
            if (input == AzureVmSize::AzureVmSizeStandard_D8as_v4)
            {
                output = Json::Value("Standard_D8as_v4");
                return;
            }
            if (input == AzureVmSize::AzureVmSizeStandard_D16as_v4)
            {
                output = Json::Value("Standard_D16as_v4");
                return;
            }
            if (input == AzureVmSize::AzureVmSizeStandard_D2a_v4)
            {
                output = Json::Value("Standard_D2a_v4");
                return;
            }
            if (input == AzureVmSize::AzureVmSizeStandard_D4a_v4)
            {
                output = Json::Value("Standard_D4a_v4");
                return;
            }
            if (input == AzureVmSize::AzureVmSizeStandard_D8a_v4)
            {
                output = Json::Value("Standard_D8a_v4");
                return;
            }
            if (input == AzureVmSize::AzureVmSizeStandard_D16a_v4)
            {
                output = Json::Value("Standard_D16a_v4");
                return;
            }
            if (input == AzureVmSize::AzureVmSizeStandard_E2a_v4)
            {
                output = Json::Value("Standard_E2a_v4");
                return;
            }
            if (input == AzureVmSize::AzureVmSizeStandard_E4a_v4)
            {
                output = Json::Value("Standard_E4a_v4");
                return;
            }
            if (input == AzureVmSize::AzureVmSizeStandard_E8a_v4)
            {
                output = Json::Value("Standard_E8a_v4");
                return;
            }
            if (input == AzureVmSize::AzureVmSizeStandard_E16a_v4)
            {
                output = Json::Value("Standard_E16a_v4");
                return;
            }
            if (input == AzureVmSize::AzureVmSizeStandard_E2as_v4)
            {
                output = Json::Value("Standard_E2as_v4");
                return;
            }
            if (input == AzureVmSize::AzureVmSizeStandard_E4as_v4)
            {
                output = Json::Value("Standard_E4as_v4");
                return;
            }
            if (input == AzureVmSize::AzureVmSizeStandard_E8as_v4)
            {
                output = Json::Value("Standard_E8as_v4");
                return;
            }
            if (input == AzureVmSize::AzureVmSizeStandard_E16as_v4)
            {
                output = Json::Value("Standard_E16as_v4");
                return;
            }
            if (input == AzureVmSize::AzureVmSizeStandard_D2s_v3)
            {
                output = Json::Value("Standard_D2s_v3");
                return;
            }
            if (input == AzureVmSize::AzureVmSizeStandard_D4s_v3)
            {
                output = Json::Value("Standard_D4s_v3");
                return;
            }
            if (input == AzureVmSize::AzureVmSizeStandard_D8s_v3)
            {
                output = Json::Value("Standard_D8s_v3");
                return;
            }
            if (input == AzureVmSize::AzureVmSizeStandard_D16s_v3)
            {
                output = Json::Value("Standard_D16s_v3");
                return;
            }
            if (input == AzureVmSize::AzureVmSizeStandard_DS1_v2)
            {
                output = Json::Value("Standard_DS1_v2");
                return;
            }
            if (input == AzureVmSize::AzureVmSizeStandard_DS2_v2)
            {
                output = Json::Value("Standard_DS2_v2");
                return;
            }
            if (input == AzureVmSize::AzureVmSizeStandard_DS3_v2)
            {
                output = Json::Value("Standard_DS3_v2");
                return;
            }
            if (input == AzureVmSize::AzureVmSizeStandard_DS4_v2)
            {
                output = Json::Value("Standard_DS4_v2");
                return;
            }
            if (input == AzureVmSize::AzureVmSizeStandard_DS5_v2)
            {
                output = Json::Value("Standard_DS5_v2");
                return;
            }
            if (input == AzureVmSize::AzureVmSizeStandard_NC4as_T4_v3)
            {
                output = Json::Value("Standard_NC4as_T4_v3");
                return;
            }
            if (input == AzureVmSize::AzureVmSizeStandard_D2d_v4)
            {
                output = Json::Value("Standard_D2d_v4");
                return;
            }
            if (input == AzureVmSize::AzureVmSizeStandard_D4d_v4)
            {
                output = Json::Value("Standard_D4d_v4");
                return;
            }
            if (input == AzureVmSize::AzureVmSizeStandard_D8d_v4)
            {
                output = Json::Value("Standard_D8d_v4");
                return;
            }
            if (input == AzureVmSize::AzureVmSizeStandard_D16d_v4)
            {
                output = Json::Value("Standard_D16d_v4");
                return;
            }
            if (input == AzureVmSize::AzureVmSizeStandard_D2ds_v4)
            {
                output = Json::Value("Standard_D2ds_v4");
                return;
            }
            if (input == AzureVmSize::AzureVmSizeStandard_D4ds_v4)
            {
                output = Json::Value("Standard_D4ds_v4");
                return;
            }
            if (input == AzureVmSize::AzureVmSizeStandard_D8ds_v4)
            {
                output = Json::Value("Standard_D8ds_v4");
                return;
            }
            if (input == AzureVmSize::AzureVmSizeStandard_D16ds_v4)
            {
                output = Json::Value("Standard_D16ds_v4");
                return;
            }
            if (input == AzureVmSize::AzureVmSizeStandard_HB120_16rs_v3)
            {
                output = Json::Value("Standard_HB120_16rs_v3");
                return;
            }
            if (input == AzureVmSize::AzureVmSizeStandard_HB120_32rs_v3)
            {
                output = Json::Value("Standard_HB120_32rs_v3");
                return;
            }
            if (input == AzureVmSize::AzureVmSizeStandard_HB120_64rs_v3)
            {
                output = Json::Value("Standard_HB120_64rs_v3");
                return;
            }
            if (input == AzureVmSize::AzureVmSizeStandard_HB120_96rs_v3)
            {
                output = Json::Value("Standard_HB120_96rs_v3");
                return;
            }
            if (input == AzureVmSize::AzureVmSizeStandard_HB120rs_v3)
            {
                output = Json::Value("Standard_HB120rs_v3");
                return;
            }
        }
        inline void FromJsonEnum(const Json::Value& input, AzureVmSize& output)
        {
            if (!input.isString())
            {
                return;
            }
            const std::string& inputStr = input.asString();
            if (inputStr == "Standard_A1")
            {
                output = AzureVmSize::AzureVmSizeStandard_A1;
                return;
            }
            if (inputStr == "Standard_A2")
            {
                output = AzureVmSize::AzureVmSizeStandard_A2;
                return;
            }
            if (inputStr == "Standard_A3")
            {
                output = AzureVmSize::AzureVmSizeStandard_A3;
                return;
            }
            if (inputStr == "Standard_A4")
            {
                output = AzureVmSize::AzureVmSizeStandard_A4;
                return;
            }
            if (inputStr == "Standard_A1_v2")
            {
                output = AzureVmSize::AzureVmSizeStandard_A1_v2;
                return;
            }
            if (inputStr == "Standard_A2_v2")
            {
                output = AzureVmSize::AzureVmSizeStandard_A2_v2;
                return;
            }
            if (inputStr == "Standard_A4_v2")
            {
                output = AzureVmSize::AzureVmSizeStandard_A4_v2;
                return;
            }
            if (inputStr == "Standard_A8_v2")
            {
                output = AzureVmSize::AzureVmSizeStandard_A8_v2;
                return;
            }
            if (inputStr == "Standard_D1_v2")
            {
                output = AzureVmSize::AzureVmSizeStandard_D1_v2;
                return;
            }
            if (inputStr == "Standard_D2_v2")
            {
                output = AzureVmSize::AzureVmSizeStandard_D2_v2;
                return;
            }
            if (inputStr == "Standard_D3_v2")
            {
                output = AzureVmSize::AzureVmSizeStandard_D3_v2;
                return;
            }
            if (inputStr == "Standard_D4_v2")
            {
                output = AzureVmSize::AzureVmSizeStandard_D4_v2;
                return;
            }
            if (inputStr == "Standard_D5_v2")
            {
                output = AzureVmSize::AzureVmSizeStandard_D5_v2;
                return;
            }
            if (inputStr == "Standard_D2_v3")
            {
                output = AzureVmSize::AzureVmSizeStandard_D2_v3;
                return;
            }
            if (inputStr == "Standard_D4_v3")
            {
                output = AzureVmSize::AzureVmSizeStandard_D4_v3;
                return;
            }
            if (inputStr == "Standard_D8_v3")
            {
                output = AzureVmSize::AzureVmSizeStandard_D8_v3;
                return;
            }
            if (inputStr == "Standard_D16_v3")
            {
                output = AzureVmSize::AzureVmSizeStandard_D16_v3;
                return;
            }
            if (inputStr == "Standard_F1")
            {
                output = AzureVmSize::AzureVmSizeStandard_F1;
                return;
            }
            if (inputStr == "Standard_F2")
            {
                output = AzureVmSize::AzureVmSizeStandard_F2;
                return;
            }
            if (inputStr == "Standard_F4")
            {
                output = AzureVmSize::AzureVmSizeStandard_F4;
                return;
            }
            if (inputStr == "Standard_F8")
            {
                output = AzureVmSize::AzureVmSizeStandard_F8;
                return;
            }
            if (inputStr == "Standard_F16")
            {
                output = AzureVmSize::AzureVmSizeStandard_F16;
                return;
            }
            if (inputStr == "Standard_F2s_v2")
            {
                output = AzureVmSize::AzureVmSizeStandard_F2s_v2;
                return;
            }
            if (inputStr == "Standard_F4s_v2")
            {
                output = AzureVmSize::AzureVmSizeStandard_F4s_v2;
                return;
            }
            if (inputStr == "Standard_F8s_v2")
            {
                output = AzureVmSize::AzureVmSizeStandard_F8s_v2;
                return;
            }
            if (inputStr == "Standard_F16s_v2")
            {
                output = AzureVmSize::AzureVmSizeStandard_F16s_v2;
                return;
            }
            if (inputStr == "Standard_D2as_v4")
            {
                output = AzureVmSize::AzureVmSizeStandard_D2as_v4;
                return;
            }
            if (inputStr == "Standard_D4as_v4")
            {
                output = AzureVmSize::AzureVmSizeStandard_D4as_v4;
                return;
            }
            if (inputStr == "Standard_D8as_v4")
            {
                output = AzureVmSize::AzureVmSizeStandard_D8as_v4;
                return;
            }
            if (inputStr == "Standard_D16as_v4")
            {
                output = AzureVmSize::AzureVmSizeStandard_D16as_v4;
                return;
            }
            if (inputStr == "Standard_D2a_v4")
            {
                output = AzureVmSize::AzureVmSizeStandard_D2a_v4;
                return;
            }
            if (inputStr == "Standard_D4a_v4")
            {
                output = AzureVmSize::AzureVmSizeStandard_D4a_v4;
                return;
            }
            if (inputStr == "Standard_D8a_v4")
            {
                output = AzureVmSize::AzureVmSizeStandard_D8a_v4;
                return;
            }
            if (inputStr == "Standard_D16a_v4")
            {
                output = AzureVmSize::AzureVmSizeStandard_D16a_v4;
                return;
            }
            if (inputStr == "Standard_E2a_v4")
            {
                output = AzureVmSize::AzureVmSizeStandard_E2a_v4;
                return;
            }
            if (inputStr == "Standard_E4a_v4")
            {
                output = AzureVmSize::AzureVmSizeStandard_E4a_v4;
                return;
            }
            if (inputStr == "Standard_E8a_v4")
            {
                output = AzureVmSize::AzureVmSizeStandard_E8a_v4;
                return;
            }
            if (inputStr == "Standard_E16a_v4")
            {
                output = AzureVmSize::AzureVmSizeStandard_E16a_v4;
                return;
            }
            if (inputStr == "Standard_E2as_v4")
            {
                output = AzureVmSize::AzureVmSizeStandard_E2as_v4;
                return;
            }
            if (inputStr == "Standard_E4as_v4")
            {
                output = AzureVmSize::AzureVmSizeStandard_E4as_v4;
                return;
            }
            if (inputStr == "Standard_E8as_v4")
            {
                output = AzureVmSize::AzureVmSizeStandard_E8as_v4;
                return;
            }
            if (inputStr == "Standard_E16as_v4")
            {
                output = AzureVmSize::AzureVmSizeStandard_E16as_v4;
                return;
            }
            if (inputStr == "Standard_D2s_v3")
            {
                output = AzureVmSize::AzureVmSizeStandard_D2s_v3;
                return;
            }
            if (inputStr == "Standard_D4s_v3")
            {
                output = AzureVmSize::AzureVmSizeStandard_D4s_v3;
                return;
            }
            if (inputStr == "Standard_D8s_v3")
            {
                output = AzureVmSize::AzureVmSizeStandard_D8s_v3;
                return;
            }
            if (inputStr == "Standard_D16s_v3")
            {
                output = AzureVmSize::AzureVmSizeStandard_D16s_v3;
                return;
            }
            if (inputStr == "Standard_DS1_v2")
            {
                output = AzureVmSize::AzureVmSizeStandard_DS1_v2;
                return;
            }
            if (inputStr == "Standard_DS2_v2")
            {
                output = AzureVmSize::AzureVmSizeStandard_DS2_v2;
                return;
            }
            if (inputStr == "Standard_DS3_v2")
            {
                output = AzureVmSize::AzureVmSizeStandard_DS3_v2;
                return;
            }
            if (inputStr == "Standard_DS4_v2")
            {
                output = AzureVmSize::AzureVmSizeStandard_DS4_v2;
                return;
            }
            if (inputStr == "Standard_DS5_v2")
            {
                output = AzureVmSize::AzureVmSizeStandard_DS5_v2;
                return;
            }
            if (inputStr == "Standard_NC4as_T4_v3")
            {
                output = AzureVmSize::AzureVmSizeStandard_NC4as_T4_v3;
                return;
            }
            if (inputStr == "Standard_D2d_v4")
            {
                output = AzureVmSize::AzureVmSizeStandard_D2d_v4;
                return;
            }
            if (inputStr == "Standard_D4d_v4")
            {
                output = AzureVmSize::AzureVmSizeStandard_D4d_v4;
                return;
            }
            if (inputStr == "Standard_D8d_v4")
            {
                output = AzureVmSize::AzureVmSizeStandard_D8d_v4;
                return;
            }
            if (inputStr == "Standard_D16d_v4")
            {
                output = AzureVmSize::AzureVmSizeStandard_D16d_v4;
                return;
            }
            if (inputStr == "Standard_D2ds_v4")
            {
                output = AzureVmSize::AzureVmSizeStandard_D2ds_v4;
                return;
            }
            if (inputStr == "Standard_D4ds_v4")
            {
                output = AzureVmSize::AzureVmSizeStandard_D4ds_v4;
                return;
            }
            if (inputStr == "Standard_D8ds_v4")
            {
                output = AzureVmSize::AzureVmSizeStandard_D8ds_v4;
                return;
            }
            if (inputStr == "Standard_D16ds_v4")
            {
                output = AzureVmSize::AzureVmSizeStandard_D16ds_v4;
                return;
            }
            if (inputStr == "Standard_HB120_16rs_v3")
            {
                output = AzureVmSize::AzureVmSizeStandard_HB120_16rs_v3;
                return;
            }
            if (inputStr == "Standard_HB120_32rs_v3")
            {
                output = AzureVmSize::AzureVmSizeStandard_HB120_32rs_v3;
                return;
            }
            if (inputStr == "Standard_HB120_64rs_v3")
            {
                output = AzureVmSize::AzureVmSizeStandard_HB120_64rs_v3;
                return;
            }
            if (inputStr == "Standard_HB120_96rs_v3")
            {
                output = AzureVmSize::AzureVmSizeStandard_HB120_96rs_v3;
                return;
            }
            if (inputStr == "Standard_HB120rs_v3")
            {
                output = AzureVmSize::AzureVmSizeStandard_HB120rs_v3;
                return;
            }
        }

        enum class CancellationReason
        {
            CancellationReasonRequested,
            CancellationReasonInternal,
            CancellationReasonTimeout
        };

        inline void ToJsonEnum(const CancellationReason input, Json::Value& output)
        {
            if (input == CancellationReason::CancellationReasonRequested)
            {
                output = Json::Value("Requested");
                return;
            }
            if (input == CancellationReason::CancellationReasonInternal)
            {
                output = Json::Value("Internal");
                return;
            }
            if (input == CancellationReason::CancellationReasonTimeout)
            {
                output = Json::Value("Timeout");
                return;
            }
        }
        inline void FromJsonEnum(const Json::Value& input, CancellationReason& output)
        {
            if (!input.isString())
            {
                return;
            }
            const std::string& inputStr = input.asString();
            if (inputStr == "Requested")
            {
                output = CancellationReason::CancellationReasonRequested;
                return;
            }
            if (inputStr == "Internal")
            {
                output = CancellationReason::CancellationReasonInternal;
                return;
            }
            if (inputStr == "Timeout")
            {
                output = CancellationReason::CancellationReasonTimeout;
                return;
            }
        }

        enum class ContainerFlavor
        {
            ContainerFlavorManagedWindowsServerCore,
            ContainerFlavorCustomLinux,
            ContainerFlavorManagedWindowsServerCorePreview,
            ContainerFlavorInvalid
        };

        inline void ToJsonEnum(const ContainerFlavor input, Json::Value& output)
        {
            if (input == ContainerFlavor::ContainerFlavorManagedWindowsServerCore)
            {
                output = Json::Value("ManagedWindowsServerCore");
                return;
            }
            if (input == ContainerFlavor::ContainerFlavorCustomLinux)
            {
                output = Json::Value("CustomLinux");
                return;
            }
            if (input == ContainerFlavor::ContainerFlavorManagedWindowsServerCorePreview)
            {
                output = Json::Value("ManagedWindowsServerCorePreview");
                return;
            }
            if (input == ContainerFlavor::ContainerFlavorInvalid)
            {
                output = Json::Value("Invalid");
                return;
            }
        }
        inline void FromJsonEnum(const Json::Value& input, ContainerFlavor& output)
        {
            if (!input.isString())
            {
                return;
            }
            const std::string& inputStr = input.asString();
            if (inputStr == "ManagedWindowsServerCore")
            {
                output = ContainerFlavor::ContainerFlavorManagedWindowsServerCore;
                return;
            }
            if (inputStr == "CustomLinux")
            {
                output = ContainerFlavor::ContainerFlavorCustomLinux;
                return;
            }
            if (inputStr == "ManagedWindowsServerCorePreview")
            {
                output = ContainerFlavor::ContainerFlavorManagedWindowsServerCorePreview;
                return;
            }
            if (inputStr == "Invalid")
            {
                output = ContainerFlavor::ContainerFlavorInvalid;
                return;
            }
        }

        enum class OsPlatform
        {
            OsPlatformWindows,
            OsPlatformLinux
        };

        inline void ToJsonEnum(const OsPlatform input, Json::Value& output)
        {
            if (input == OsPlatform::OsPlatformWindows)
            {
                output = Json::Value("Windows");
                return;
            }
            if (input == OsPlatform::OsPlatformLinux)
            {
                output = Json::Value("Linux");
                return;
            }
        }
        inline void FromJsonEnum(const Json::Value& input, OsPlatform& output)
        {
            if (!input.isString())
            {
                return;
            }
            const std::string& inputStr = input.asString();
            if (inputStr == "Windows")
            {
                output = OsPlatform::OsPlatformWindows;
                return;
            }
            if (inputStr == "Linux")
            {
                output = OsPlatform::OsPlatformLinux;
                return;
            }
        }

        enum class ProtocolType
        {
            ProtocolTypeTCP,
            ProtocolTypeUDP
        };

        inline void ToJsonEnum(const ProtocolType input, Json::Value& output)
        {
            if (input == ProtocolType::ProtocolTypeTCP)
            {
                output = Json::Value("TCP");
                return;
            }
            if (input == ProtocolType::ProtocolTypeUDP)
            {
                output = Json::Value("UDP");
                return;
            }
        }
        inline void FromJsonEnum(const Json::Value& input, ProtocolType& output)
        {
            if (!input.isString())
            {
                return;
            }
            const std::string& inputStr = input.asString();
            if (inputStr == "TCP")
            {
                output = ProtocolType::ProtocolTypeTCP;
                return;
            }
            if (inputStr == "UDP")
            {
                output = ProtocolType::ProtocolTypeUDP;
                return;
            }
        }

        enum class ServerType
        {
            ServerTypeContainer,
            ServerTypeProcess
        };

        inline void ToJsonEnum(const ServerType input, Json::Value& output)
        {
            if (input == ServerType::ServerTypeContainer)
            {
                output = Json::Value("Container");
                return;
            }
            if (input == ServerType::ServerTypeProcess)
            {
                output = Json::Value("Process");
                return;
            }
        }
        inline void FromJsonEnum(const Json::Value& input, ServerType& output)
        {
            if (!input.isString())
            {
                return;
            }
            const std::string& inputStr = input.asString();
            if (inputStr == "Container")
            {
                output = ServerType::ServerTypeContainer;
                return;
            }
            if (inputStr == "Process")
            {
                output = ServerType::ServerTypeProcess;
                return;
            }
        }

        enum class TitleMultiplayerServerEnabledStatus
        {
            TitleMultiplayerServerEnabledStatusInitializing,
            TitleMultiplayerServerEnabledStatusEnabled,
            TitleMultiplayerServerEnabledStatusDisabled
        };

        inline void ToJsonEnum(const TitleMultiplayerServerEnabledStatus input, Json::Value& output)
        {
            if (input == TitleMultiplayerServerEnabledStatus::TitleMultiplayerServerEnabledStatusInitializing)
            {
                output = Json::Value("Initializing");
                return;
            }
            if (input == TitleMultiplayerServerEnabledStatus::TitleMultiplayerServerEnabledStatusEnabled)
            {
                output = Json::Value("Enabled");
                return;
            }
            if (input == TitleMultiplayerServerEnabledStatus::TitleMultiplayerServerEnabledStatusDisabled)
            {
                output = Json::Value("Disabled");
                return;
            }
        }
        inline void FromJsonEnum(const Json::Value& input, TitleMultiplayerServerEnabledStatus& output)
        {
            if (!input.isString())
            {
                return;
            }
            const std::string& inputStr = input.asString();
            if (inputStr == "Initializing")
            {
                output = TitleMultiplayerServerEnabledStatus::TitleMultiplayerServerEnabledStatusInitializing;
                return;
            }
            if (inputStr == "Enabled")
            {
                output = TitleMultiplayerServerEnabledStatus::TitleMultiplayerServerEnabledStatusEnabled;
                return;
            }
            if (inputStr == "Disabled")
            {
                output = TitleMultiplayerServerEnabledStatus::TitleMultiplayerServerEnabledStatusDisabled;
                return;
            }
        }

        // Multiplayer Classes
        struct AssetReference : public PlayFabBaseModel
        {
            std::string FileName;
            std::string MountPath;

            AssetReference() :
                PlayFabBaseModel(),
                FileName(),
                MountPath()
            {}

            AssetReference(const AssetReference& src) :
                PlayFabBaseModel(),
                FileName(src.FileName),
                MountPath(src.MountPath)
            {}

            ~AssetReference() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["FileName"], FileName);
                FromJsonUtilS(input["MountPath"], MountPath);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_FileName; ToJsonUtilS(FileName, each_FileName); output["FileName"] = each_FileName;
                Json::Value each_MountPath; ToJsonUtilS(MountPath, each_MountPath); output["MountPath"] = each_MountPath;
                return output;
            }
        };

        struct AssetReferenceParams : public PlayFabBaseModel
        {
            std::string FileName;
            std::string MountPath;

            AssetReferenceParams() :
                PlayFabBaseModel(),
                FileName(),
                MountPath()
            {}

            AssetReferenceParams(const AssetReferenceParams& src) :
                PlayFabBaseModel(),
                FileName(src.FileName),
                MountPath(src.MountPath)
            {}

            ~AssetReferenceParams() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["FileName"], FileName);
                FromJsonUtilS(input["MountPath"], MountPath);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_FileName; ToJsonUtilS(FileName, each_FileName); output["FileName"] = each_FileName;
                Json::Value each_MountPath; ToJsonUtilS(MountPath, each_MountPath); output["MountPath"] = each_MountPath;
                return output;
            }
        };

        struct AssetSummary : public PlayFabBaseModel
        {
            std::string FileName;
            std::map<std::string, std::string> Metadata;

            AssetSummary() :
                PlayFabBaseModel(),
                FileName(),
                Metadata()
            {}

            AssetSummary(const AssetSummary& src) :
                PlayFabBaseModel(),
                FileName(src.FileName),
                Metadata(src.Metadata)
            {}

            ~AssetSummary() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["FileName"], FileName);
                FromJsonUtilS(input["Metadata"], Metadata);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_FileName; ToJsonUtilS(FileName, each_FileName); output["FileName"] = each_FileName;
                Json::Value each_Metadata; ToJsonUtilS(Metadata, each_Metadata); output["Metadata"] = each_Metadata;
                return output;
            }
        };

        struct BuildSelectionCriterion : public PlayFabBaseModel
        {
            std::map<std::string, Uint32> BuildWeightDistribution;

            BuildSelectionCriterion() :
                PlayFabBaseModel(),
                BuildWeightDistribution()
            {}

            BuildSelectionCriterion(const BuildSelectionCriterion& src) :
                PlayFabBaseModel(),
                BuildWeightDistribution(src.BuildWeightDistribution)
            {}

            ~BuildSelectionCriterion() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilP(input["BuildWeightDistribution"], BuildWeightDistribution);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_BuildWeightDistribution; ToJsonUtilP(BuildWeightDistribution, each_BuildWeightDistribution); output["BuildWeightDistribution"] = each_BuildWeightDistribution;
                return output;
            }
        };

        struct BuildAliasDetailsResponse : public PlayFabResultCommon
        {
            std::string AliasId;
            std::string AliasName;
            std::list<BuildSelectionCriterion> BuildSelectionCriteria;

            BuildAliasDetailsResponse() :
                PlayFabResultCommon(),
                AliasId(),
                AliasName(),
                BuildSelectionCriteria()
            {}

            BuildAliasDetailsResponse(const BuildAliasDetailsResponse& src) :
                PlayFabResultCommon(),
                AliasId(src.AliasId),
                AliasName(src.AliasName),
                BuildSelectionCriteria(src.BuildSelectionCriteria)
            {}

            ~BuildAliasDetailsResponse() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["AliasId"], AliasId);
                FromJsonUtilS(input["AliasName"], AliasName);
                FromJsonUtilO(input["BuildSelectionCriteria"], BuildSelectionCriteria);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_AliasId; ToJsonUtilS(AliasId, each_AliasId); output["AliasId"] = each_AliasId;
                Json::Value each_AliasName; ToJsonUtilS(AliasName, each_AliasName); output["AliasName"] = each_AliasName;
                Json::Value each_BuildSelectionCriteria; ToJsonUtilO(BuildSelectionCriteria, each_BuildSelectionCriteria); output["BuildSelectionCriteria"] = each_BuildSelectionCriteria;
                return output;
            }
        };

        struct BuildAliasParams : public PlayFabBaseModel
        {
            std::string AliasId;

            BuildAliasParams() :
                PlayFabBaseModel(),
                AliasId()
            {}

            BuildAliasParams(const BuildAliasParams& src) :
                PlayFabBaseModel(),
                AliasId(src.AliasId)
            {}

            ~BuildAliasParams() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["AliasId"], AliasId);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_AliasId; ToJsonUtilS(AliasId, each_AliasId); output["AliasId"] = each_AliasId;
                return output;
            }
        };

        struct CurrentServerStats : public PlayFabBaseModel
        {
            Int32 Active;
            Int32 Propping;
            Int32 StandingBy;
            Int32 Total;

            CurrentServerStats() :
                PlayFabBaseModel(),
                Active(),
                Propping(),
                StandingBy(),
                Total()
            {}

            CurrentServerStats(const CurrentServerStats& src) :
                PlayFabBaseModel(),
                Active(src.Active),
                Propping(src.Propping),
                StandingBy(src.StandingBy),
                Total(src.Total)
            {}

            ~CurrentServerStats() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilP(input["Active"], Active);
                FromJsonUtilP(input["Propping"], Propping);
                FromJsonUtilP(input["StandingBy"], StandingBy);
                FromJsonUtilP(input["Total"], Total);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_Active; ToJsonUtilP(Active, each_Active); output["Active"] = each_Active;
                Json::Value each_Propping; ToJsonUtilP(Propping, each_Propping); output["Propping"] = each_Propping;
                Json::Value each_StandingBy; ToJsonUtilP(StandingBy, each_StandingBy); output["StandingBy"] = each_StandingBy;
                Json::Value each_Total; ToJsonUtilP(Total, each_Total); output["Total"] = each_Total;
                return output;
            }
        };

        struct DynamicStandbyThreshold : public PlayFabBaseModel
        {
            double Multiplier;
            double TriggerThresholdPercentage;

            DynamicStandbyThreshold() :
                PlayFabBaseModel(),
                Multiplier(),
                TriggerThresholdPercentage()
            {}

            DynamicStandbyThreshold(const DynamicStandbyThreshold& src) :
                PlayFabBaseModel(),
                Multiplier(src.Multiplier),
                TriggerThresholdPercentage(src.TriggerThresholdPercentage)
            {}

            ~DynamicStandbyThreshold() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilP(input["Multiplier"], Multiplier);
                FromJsonUtilP(input["TriggerThresholdPercentage"], TriggerThresholdPercentage);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_Multiplier; ToJsonUtilP(Multiplier, each_Multiplier); output["Multiplier"] = each_Multiplier;
                Json::Value each_TriggerThresholdPercentage; ToJsonUtilP(TriggerThresholdPercentage, each_TriggerThresholdPercentage); output["TriggerThresholdPercentage"] = each_TriggerThresholdPercentage;
                return output;
            }
        };

        struct DynamicStandbySettings : public PlayFabBaseModel
        {
            std::list<DynamicStandbyThreshold> DynamicFloorMultiplierThresholds;
            bool IsEnabled;
            Boxed<Int32> RampDownSeconds;

            DynamicStandbySettings() :
                PlayFabBaseModel(),
                DynamicFloorMultiplierThresholds(),
                IsEnabled(),
                RampDownSeconds()
            {}

            DynamicStandbySettings(const DynamicStandbySettings& src) :
                PlayFabBaseModel(),
                DynamicFloorMultiplierThresholds(src.DynamicFloorMultiplierThresholds),
                IsEnabled(src.IsEnabled),
                RampDownSeconds(src.RampDownSeconds)
            {}

            ~DynamicStandbySettings() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilO(input["DynamicFloorMultiplierThresholds"], DynamicFloorMultiplierThresholds);
                FromJsonUtilP(input["IsEnabled"], IsEnabled);
                FromJsonUtilP(input["RampDownSeconds"], RampDownSeconds);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_DynamicFloorMultiplierThresholds; ToJsonUtilO(DynamicFloorMultiplierThresholds, each_DynamicFloorMultiplierThresholds); output["DynamicFloorMultiplierThresholds"] = each_DynamicFloorMultiplierThresholds;
                Json::Value each_IsEnabled; ToJsonUtilP(IsEnabled, each_IsEnabled); output["IsEnabled"] = each_IsEnabled;
                Json::Value each_RampDownSeconds; ToJsonUtilP(RampDownSeconds, each_RampDownSeconds); output["RampDownSeconds"] = each_RampDownSeconds;
                return output;
            }
        };

        struct Schedule : public PlayFabBaseModel
        {
            std::string Description;
            time_t EndTime;
            bool IsDisabled;
            bool IsRecurringWeekly;
            time_t StartTime;
            Int32 TargetStandby;

            Schedule() :
                PlayFabBaseModel(),
                Description(),
                EndTime(),
                IsDisabled(),
                IsRecurringWeekly(),
                StartTime(),
                TargetStandby()
            {}

            Schedule(const Schedule& src) :
                PlayFabBaseModel(),
                Description(src.Description),
                EndTime(src.EndTime),
                IsDisabled(src.IsDisabled),
                IsRecurringWeekly(src.IsRecurringWeekly),
                StartTime(src.StartTime),
                TargetStandby(src.TargetStandby)
            {}

            ~Schedule() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["Description"], Description);
                FromJsonUtilT(input["EndTime"], EndTime);
                FromJsonUtilP(input["IsDisabled"], IsDisabled);
                FromJsonUtilP(input["IsRecurringWeekly"], IsRecurringWeekly);
                FromJsonUtilT(input["StartTime"], StartTime);
                FromJsonUtilP(input["TargetStandby"], TargetStandby);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_Description; ToJsonUtilS(Description, each_Description); output["Description"] = each_Description;
                Json::Value each_EndTime; ToJsonUtilT(EndTime, each_EndTime); output["EndTime"] = each_EndTime;
                Json::Value each_IsDisabled; ToJsonUtilP(IsDisabled, each_IsDisabled); output["IsDisabled"] = each_IsDisabled;
                Json::Value each_IsRecurringWeekly; ToJsonUtilP(IsRecurringWeekly, each_IsRecurringWeekly); output["IsRecurringWeekly"] = each_IsRecurringWeekly;
                Json::Value each_StartTime; ToJsonUtilT(StartTime, each_StartTime); output["StartTime"] = each_StartTime;
                Json::Value each_TargetStandby; ToJsonUtilP(TargetStandby, each_TargetStandby); output["TargetStandby"] = each_TargetStandby;
                return output;
            }
        };

        struct ScheduledStandbySettings : public PlayFabBaseModel
        {
            bool IsEnabled;
            std::list<Schedule> ScheduleList;

            ScheduledStandbySettings() :
                PlayFabBaseModel(),
                IsEnabled(),
                ScheduleList()
            {}

            ScheduledStandbySettings(const ScheduledStandbySettings& src) :
                PlayFabBaseModel(),
                IsEnabled(src.IsEnabled),
                ScheduleList(src.ScheduleList)
            {}

            ~ScheduledStandbySettings() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilP(input["IsEnabled"], IsEnabled);
                FromJsonUtilO(input["ScheduleList"], ScheduleList);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_IsEnabled; ToJsonUtilP(IsEnabled, each_IsEnabled); output["IsEnabled"] = each_IsEnabled;
                Json::Value each_ScheduleList; ToJsonUtilO(ScheduleList, each_ScheduleList); output["ScheduleList"] = each_ScheduleList;
                return output;
            }
        };

        struct BuildRegion : public PlayFabBaseModel
        {
            Boxed<CurrentServerStats> pfCurrentServerStats;
            Boxed<DynamicStandbySettings> pfDynamicStandbySettings;
            Int32 MaxServers;
            Boxed<Int32> MultiplayerServerCountPerVm;
            std::string Region;
            Boxed<ScheduledStandbySettings> pfScheduledStandbySettings;
            Int32 StandbyServers;
            std::string Status;
            Boxed<AzureVmSize> VmSize;

            BuildRegion() :
                PlayFabBaseModel(),
                pfCurrentServerStats(),
                pfDynamicStandbySettings(),
                MaxServers(),
                MultiplayerServerCountPerVm(),
                Region(),
                pfScheduledStandbySettings(),
                StandbyServers(),
                Status(),
                VmSize()
            {}

            BuildRegion(const BuildRegion& src) :
                PlayFabBaseModel(),
                pfCurrentServerStats(src.pfCurrentServerStats),
                pfDynamicStandbySettings(src.pfDynamicStandbySettings),
                MaxServers(src.MaxServers),
                MultiplayerServerCountPerVm(src.MultiplayerServerCountPerVm),
                Region(src.Region),
                pfScheduledStandbySettings(src.pfScheduledStandbySettings),
                StandbyServers(src.StandbyServers),
                Status(src.Status),
                VmSize(src.VmSize)
            {}

            ~BuildRegion() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilO(input["CurrentServerStats"], pfCurrentServerStats);
                FromJsonUtilO(input["DynamicStandbySettings"], pfDynamicStandbySettings);
                FromJsonUtilP(input["MaxServers"], MaxServers);
                FromJsonUtilP(input["MultiplayerServerCountPerVm"], MultiplayerServerCountPerVm);
                FromJsonUtilS(input["Region"], Region);
                FromJsonUtilO(input["ScheduledStandbySettings"], pfScheduledStandbySettings);
                FromJsonUtilP(input["StandbyServers"], StandbyServers);
                FromJsonUtilS(input["Status"], Status);
                FromJsonUtilE(input["VmSize"], VmSize);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_pfCurrentServerStats; ToJsonUtilO(pfCurrentServerStats, each_pfCurrentServerStats); output["CurrentServerStats"] = each_pfCurrentServerStats;
                Json::Value each_pfDynamicStandbySettings; ToJsonUtilO(pfDynamicStandbySettings, each_pfDynamicStandbySettings); output["DynamicStandbySettings"] = each_pfDynamicStandbySettings;
                Json::Value each_MaxServers; ToJsonUtilP(MaxServers, each_MaxServers); output["MaxServers"] = each_MaxServers;
                Json::Value each_MultiplayerServerCountPerVm; ToJsonUtilP(MultiplayerServerCountPerVm, each_MultiplayerServerCountPerVm); output["MultiplayerServerCountPerVm"] = each_MultiplayerServerCountPerVm;
                Json::Value each_Region; ToJsonUtilS(Region, each_Region); output["Region"] = each_Region;
                Json::Value each_pfScheduledStandbySettings; ToJsonUtilO(pfScheduledStandbySettings, each_pfScheduledStandbySettings); output["ScheduledStandbySettings"] = each_pfScheduledStandbySettings;
                Json::Value each_StandbyServers; ToJsonUtilP(StandbyServers, each_StandbyServers); output["StandbyServers"] = each_StandbyServers;
                Json::Value each_Status; ToJsonUtilS(Status, each_Status); output["Status"] = each_Status;
                Json::Value each_VmSize; ToJsonUtilE(VmSize, each_VmSize); output["VmSize"] = each_VmSize;
                return output;
            }
        };

        struct BuildRegionParams : public PlayFabBaseModel
        {
            Boxed<DynamicStandbySettings> pfDynamicStandbySettings;
            Int32 MaxServers;
            Boxed<Int32> MultiplayerServerCountPerVm;
            std::string Region;
            Boxed<ScheduledStandbySettings> pfScheduledStandbySettings;
            Int32 StandbyServers;
            Boxed<AzureVmSize> VmSize;

            BuildRegionParams() :
                PlayFabBaseModel(),
                pfDynamicStandbySettings(),
                MaxServers(),
                MultiplayerServerCountPerVm(),
                Region(),
                pfScheduledStandbySettings(),
                StandbyServers(),
                VmSize()
            {}

            BuildRegionParams(const BuildRegionParams& src) :
                PlayFabBaseModel(),
                pfDynamicStandbySettings(src.pfDynamicStandbySettings),
                MaxServers(src.MaxServers),
                MultiplayerServerCountPerVm(src.MultiplayerServerCountPerVm),
                Region(src.Region),
                pfScheduledStandbySettings(src.pfScheduledStandbySettings),
                StandbyServers(src.StandbyServers),
                VmSize(src.VmSize)
            {}

            ~BuildRegionParams() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilO(input["DynamicStandbySettings"], pfDynamicStandbySettings);
                FromJsonUtilP(input["MaxServers"], MaxServers);
                FromJsonUtilP(input["MultiplayerServerCountPerVm"], MultiplayerServerCountPerVm);
                FromJsonUtilS(input["Region"], Region);
                FromJsonUtilO(input["ScheduledStandbySettings"], pfScheduledStandbySettings);
                FromJsonUtilP(input["StandbyServers"], StandbyServers);
                FromJsonUtilE(input["VmSize"], VmSize);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_pfDynamicStandbySettings; ToJsonUtilO(pfDynamicStandbySettings, each_pfDynamicStandbySettings); output["DynamicStandbySettings"] = each_pfDynamicStandbySettings;
                Json::Value each_MaxServers; ToJsonUtilP(MaxServers, each_MaxServers); output["MaxServers"] = each_MaxServers;
                Json::Value each_MultiplayerServerCountPerVm; ToJsonUtilP(MultiplayerServerCountPerVm, each_MultiplayerServerCountPerVm); output["MultiplayerServerCountPerVm"] = each_MultiplayerServerCountPerVm;
                Json::Value each_Region; ToJsonUtilS(Region, each_Region); output["Region"] = each_Region;
                Json::Value each_pfScheduledStandbySettings; ToJsonUtilO(pfScheduledStandbySettings, each_pfScheduledStandbySettings); output["ScheduledStandbySettings"] = each_pfScheduledStandbySettings;
                Json::Value each_StandbyServers; ToJsonUtilP(StandbyServers, each_StandbyServers); output["StandbyServers"] = each_StandbyServers;
                Json::Value each_VmSize; ToJsonUtilE(VmSize, each_VmSize); output["VmSize"] = each_VmSize;
                return output;
            }
        };

        struct BuildSummary : public PlayFabBaseModel
        {
            std::string BuildId;
            std::string BuildName;
            Boxed<time_t> CreationTime;
            std::map<std::string, std::string> Metadata;
            std::list<BuildRegion> RegionConfigurations;

            BuildSummary() :
                PlayFabBaseModel(),
                BuildId(),
                BuildName(),
                CreationTime(),
                Metadata(),
                RegionConfigurations()
            {}

            BuildSummary(const BuildSummary& src) :
                PlayFabBaseModel(),
                BuildId(src.BuildId),
                BuildName(src.BuildName),
                CreationTime(src.CreationTime),
                Metadata(src.Metadata),
                RegionConfigurations(src.RegionConfigurations)
            {}

            ~BuildSummary() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["BuildId"], BuildId);
                FromJsonUtilS(input["BuildName"], BuildName);
                FromJsonUtilT(input["CreationTime"], CreationTime);
                FromJsonUtilS(input["Metadata"], Metadata);
                FromJsonUtilO(input["RegionConfigurations"], RegionConfigurations);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_BuildId; ToJsonUtilS(BuildId, each_BuildId); output["BuildId"] = each_BuildId;
                Json::Value each_BuildName; ToJsonUtilS(BuildName, each_BuildName); output["BuildName"] = each_BuildName;
                Json::Value each_CreationTime; ToJsonUtilT(CreationTime, each_CreationTime); output["CreationTime"] = each_CreationTime;
                Json::Value each_Metadata; ToJsonUtilS(Metadata, each_Metadata); output["Metadata"] = each_Metadata;
                Json::Value each_RegionConfigurations; ToJsonUtilO(RegionConfigurations, each_RegionConfigurations); output["RegionConfigurations"] = each_RegionConfigurations;
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

        struct CancelAllMatchmakingTicketsForPlayerRequest : public PlayFabRequestCommon
        {
            std::map<std::string, std::string> CustomTags;
            Boxed<EntityKey> Entity;
            std::string QueueName;

            CancelAllMatchmakingTicketsForPlayerRequest() :
                PlayFabRequestCommon(),
                CustomTags(),
                Entity(),
                QueueName()
            {}

            CancelAllMatchmakingTicketsForPlayerRequest(const CancelAllMatchmakingTicketsForPlayerRequest& src) :
                PlayFabRequestCommon(),
                CustomTags(src.CustomTags),
                Entity(src.Entity),
                QueueName(src.QueueName)
            {}

            ~CancelAllMatchmakingTicketsForPlayerRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["CustomTags"], CustomTags);
                FromJsonUtilO(input["Entity"], Entity);
                FromJsonUtilS(input["QueueName"], QueueName);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_CustomTags; ToJsonUtilS(CustomTags, each_CustomTags); output["CustomTags"] = each_CustomTags;
                Json::Value each_Entity; ToJsonUtilO(Entity, each_Entity); output["Entity"] = each_Entity;
                Json::Value each_QueueName; ToJsonUtilS(QueueName, each_QueueName); output["QueueName"] = each_QueueName;
                return output;
            }
        };

        struct CancelAllMatchmakingTicketsForPlayerResult : public PlayFabResultCommon
        {

            CancelAllMatchmakingTicketsForPlayerResult() :
                PlayFabResultCommon()
            {}

            CancelAllMatchmakingTicketsForPlayerResult(const CancelAllMatchmakingTicketsForPlayerResult&) :
                PlayFabResultCommon()
            {}

            ~CancelAllMatchmakingTicketsForPlayerResult() = default;

            void FromJson(const Json::Value&) override
            {
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                return output;
            }
        };

        struct CancelAllServerBackfillTicketsForPlayerRequest : public PlayFabRequestCommon
        {
            std::map<std::string, std::string> CustomTags;
            EntityKey Entity;
            std::string QueueName;

            CancelAllServerBackfillTicketsForPlayerRequest() :
                PlayFabRequestCommon(),
                CustomTags(),
                Entity(),
                QueueName()
            {}

            CancelAllServerBackfillTicketsForPlayerRequest(const CancelAllServerBackfillTicketsForPlayerRequest& src) :
                PlayFabRequestCommon(),
                CustomTags(src.CustomTags),
                Entity(src.Entity),
                QueueName(src.QueueName)
            {}

            ~CancelAllServerBackfillTicketsForPlayerRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["CustomTags"], CustomTags);
                FromJsonUtilO(input["Entity"], Entity);
                FromJsonUtilS(input["QueueName"], QueueName);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_CustomTags; ToJsonUtilS(CustomTags, each_CustomTags); output["CustomTags"] = each_CustomTags;
                Json::Value each_Entity; ToJsonUtilO(Entity, each_Entity); output["Entity"] = each_Entity;
                Json::Value each_QueueName; ToJsonUtilS(QueueName, each_QueueName); output["QueueName"] = each_QueueName;
                return output;
            }
        };

        struct CancelAllServerBackfillTicketsForPlayerResult : public PlayFabResultCommon
        {

            CancelAllServerBackfillTicketsForPlayerResult() :
                PlayFabResultCommon()
            {}

            CancelAllServerBackfillTicketsForPlayerResult(const CancelAllServerBackfillTicketsForPlayerResult&) :
                PlayFabResultCommon()
            {}

            ~CancelAllServerBackfillTicketsForPlayerResult() = default;

            void FromJson(const Json::Value&) override
            {
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                return output;
            }
        };

        struct CancelMatchmakingTicketRequest : public PlayFabRequestCommon
        {
            std::map<std::string, std::string> CustomTags;
            std::string QueueName;
            std::string TicketId;

            CancelMatchmakingTicketRequest() :
                PlayFabRequestCommon(),
                CustomTags(),
                QueueName(),
                TicketId()
            {}

            CancelMatchmakingTicketRequest(const CancelMatchmakingTicketRequest& src) :
                PlayFabRequestCommon(),
                CustomTags(src.CustomTags),
                QueueName(src.QueueName),
                TicketId(src.TicketId)
            {}

            ~CancelMatchmakingTicketRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["CustomTags"], CustomTags);
                FromJsonUtilS(input["QueueName"], QueueName);
                FromJsonUtilS(input["TicketId"], TicketId);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_CustomTags; ToJsonUtilS(CustomTags, each_CustomTags); output["CustomTags"] = each_CustomTags;
                Json::Value each_QueueName; ToJsonUtilS(QueueName, each_QueueName); output["QueueName"] = each_QueueName;
                Json::Value each_TicketId; ToJsonUtilS(TicketId, each_TicketId); output["TicketId"] = each_TicketId;
                return output;
            }
        };

        struct CancelMatchmakingTicketResult : public PlayFabResultCommon
        {

            CancelMatchmakingTicketResult() :
                PlayFabResultCommon()
            {}

            CancelMatchmakingTicketResult(const CancelMatchmakingTicketResult&) :
                PlayFabResultCommon()
            {}

            ~CancelMatchmakingTicketResult() = default;

            void FromJson(const Json::Value&) override
            {
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                return output;
            }
        };

        struct CancelServerBackfillTicketRequest : public PlayFabRequestCommon
        {
            std::map<std::string, std::string> CustomTags;
            std::string QueueName;
            std::string TicketId;

            CancelServerBackfillTicketRequest() :
                PlayFabRequestCommon(),
                CustomTags(),
                QueueName(),
                TicketId()
            {}

            CancelServerBackfillTicketRequest(const CancelServerBackfillTicketRequest& src) :
                PlayFabRequestCommon(),
                CustomTags(src.CustomTags),
                QueueName(src.QueueName),
                TicketId(src.TicketId)
            {}

            ~CancelServerBackfillTicketRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["CustomTags"], CustomTags);
                FromJsonUtilS(input["QueueName"], QueueName);
                FromJsonUtilS(input["TicketId"], TicketId);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_CustomTags; ToJsonUtilS(CustomTags, each_CustomTags); output["CustomTags"] = each_CustomTags;
                Json::Value each_QueueName; ToJsonUtilS(QueueName, each_QueueName); output["QueueName"] = each_QueueName;
                Json::Value each_TicketId; ToJsonUtilS(TicketId, each_TicketId); output["TicketId"] = each_TicketId;
                return output;
            }
        };

        struct CancelServerBackfillTicketResult : public PlayFabResultCommon
        {

            CancelServerBackfillTicketResult() :
                PlayFabResultCommon()
            {}

            CancelServerBackfillTicketResult(const CancelServerBackfillTicketResult&) :
                PlayFabResultCommon()
            {}

            ~CancelServerBackfillTicketResult() = default;

            void FromJson(const Json::Value&) override
            {
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                return output;
            }
        };

        struct Certificate : public PlayFabBaseModel
        {
            std::string Base64EncodedValue;
            std::string Name;
            std::string Password;

            Certificate() :
                PlayFabBaseModel(),
                Base64EncodedValue(),
                Name(),
                Password()
            {}

            Certificate(const Certificate& src) :
                PlayFabBaseModel(),
                Base64EncodedValue(src.Base64EncodedValue),
                Name(src.Name),
                Password(src.Password)
            {}

            ~Certificate() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["Base64EncodedValue"], Base64EncodedValue);
                FromJsonUtilS(input["Name"], Name);
                FromJsonUtilS(input["Password"], Password);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_Base64EncodedValue; ToJsonUtilS(Base64EncodedValue, each_Base64EncodedValue); output["Base64EncodedValue"] = each_Base64EncodedValue;
                Json::Value each_Name; ToJsonUtilS(Name, each_Name); output["Name"] = each_Name;
                Json::Value each_Password; ToJsonUtilS(Password, each_Password); output["Password"] = each_Password;
                return output;
            }
        };

        struct CertificateSummary : public PlayFabBaseModel
        {
            std::string Name;
            std::string Thumbprint;

            CertificateSummary() :
                PlayFabBaseModel(),
                Name(),
                Thumbprint()
            {}

            CertificateSummary(const CertificateSummary& src) :
                PlayFabBaseModel(),
                Name(src.Name),
                Thumbprint(src.Thumbprint)
            {}

            ~CertificateSummary() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["Name"], Name);
                FromJsonUtilS(input["Thumbprint"], Thumbprint);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_Name; ToJsonUtilS(Name, each_Name); output["Name"] = each_Name;
                Json::Value each_Thumbprint; ToJsonUtilS(Thumbprint, each_Thumbprint); output["Thumbprint"] = each_Thumbprint;
                return output;
            }
        };

        struct ConnectedPlayer : public PlayFabBaseModel
        {
            std::string PlayerId;

            ConnectedPlayer() :
                PlayFabBaseModel(),
                PlayerId()
            {}

            ConnectedPlayer(const ConnectedPlayer& src) :
                PlayFabBaseModel(),
                PlayerId(src.PlayerId)
            {}

            ~ConnectedPlayer() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["PlayerId"], PlayerId);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_PlayerId; ToJsonUtilS(PlayerId, each_PlayerId); output["PlayerId"] = each_PlayerId;
                return output;
            }
        };

        struct ContainerImageReference : public PlayFabBaseModel
        {
            std::string ImageName;
            std::string Tag;

            ContainerImageReference() :
                PlayFabBaseModel(),
                ImageName(),
                Tag()
            {}

            ContainerImageReference(const ContainerImageReference& src) :
                PlayFabBaseModel(),
                ImageName(src.ImageName),
                Tag(src.Tag)
            {}

            ~ContainerImageReference() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["ImageName"], ImageName);
                FromJsonUtilS(input["Tag"], Tag);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_ImageName; ToJsonUtilS(ImageName, each_ImageName); output["ImageName"] = each_ImageName;
                Json::Value each_Tag; ToJsonUtilS(Tag, each_Tag); output["Tag"] = each_Tag;
                return output;
            }
        };

        struct CoreCapacity : public PlayFabBaseModel
        {
            Int32 Available;
            std::string Region;
            Int32 Total;
            Boxed<AzureVmFamily> VmFamily;

            CoreCapacity() :
                PlayFabBaseModel(),
                Available(),
                Region(),
                Total(),
                VmFamily()
            {}

            CoreCapacity(const CoreCapacity& src) :
                PlayFabBaseModel(),
                Available(src.Available),
                Region(src.Region),
                Total(src.Total),
                VmFamily(src.VmFamily)
            {}

            ~CoreCapacity() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilP(input["Available"], Available);
                FromJsonUtilS(input["Region"], Region);
                FromJsonUtilP(input["Total"], Total);
                FromJsonUtilE(input["VmFamily"], VmFamily);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_Available; ToJsonUtilP(Available, each_Available); output["Available"] = each_Available;
                Json::Value each_Region; ToJsonUtilS(Region, each_Region); output["Region"] = each_Region;
                Json::Value each_Total; ToJsonUtilP(Total, each_Total); output["Total"] = each_Total;
                Json::Value each_VmFamily; ToJsonUtilE(VmFamily, each_VmFamily); output["VmFamily"] = each_VmFamily;
                return output;
            }
        };

        struct CoreCapacityChange : public PlayFabBaseModel
        {
            Int32 NewCoreLimit;
            std::string Region;
            AzureVmFamily VmFamily;

            CoreCapacityChange() :
                PlayFabBaseModel(),
                NewCoreLimit(),
                Region(),
                VmFamily()
            {}

            CoreCapacityChange(const CoreCapacityChange& src) :
                PlayFabBaseModel(),
                NewCoreLimit(src.NewCoreLimit),
                Region(src.Region),
                VmFamily(src.VmFamily)
            {}

            ~CoreCapacityChange() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilP(input["NewCoreLimit"], NewCoreLimit);
                FromJsonUtilS(input["Region"], Region);
                FromJsonEnum(input["VmFamily"], VmFamily);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_NewCoreLimit; ToJsonUtilP(NewCoreLimit, each_NewCoreLimit); output["NewCoreLimit"] = each_NewCoreLimit;
                Json::Value each_Region; ToJsonUtilS(Region, each_Region); output["Region"] = each_Region;
                Json::Value each_VmFamily; ToJsonEnum(VmFamily, each_VmFamily); output["VmFamily"] = each_VmFamily;
                return output;
            }
        };

        struct CreateBuildAliasRequest : public PlayFabRequestCommon
        {
            std::string AliasName;
            std::list<BuildSelectionCriterion> BuildSelectionCriteria;
            std::map<std::string, std::string> CustomTags;

            CreateBuildAliasRequest() :
                PlayFabRequestCommon(),
                AliasName(),
                BuildSelectionCriteria(),
                CustomTags()
            {}

            CreateBuildAliasRequest(const CreateBuildAliasRequest& src) :
                PlayFabRequestCommon(),
                AliasName(src.AliasName),
                BuildSelectionCriteria(src.BuildSelectionCriteria),
                CustomTags(src.CustomTags)
            {}

            ~CreateBuildAliasRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["AliasName"], AliasName);
                FromJsonUtilO(input["BuildSelectionCriteria"], BuildSelectionCriteria);
                FromJsonUtilS(input["CustomTags"], CustomTags);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_AliasName; ToJsonUtilS(AliasName, each_AliasName); output["AliasName"] = each_AliasName;
                Json::Value each_BuildSelectionCriteria; ToJsonUtilO(BuildSelectionCriteria, each_BuildSelectionCriteria); output["BuildSelectionCriteria"] = each_BuildSelectionCriteria;
                Json::Value each_CustomTags; ToJsonUtilS(CustomTags, each_CustomTags); output["CustomTags"] = each_CustomTags;
                return output;
            }
        };

        struct GameCertificateReferenceParams : public PlayFabBaseModel
        {
            std::string GsdkAlias;
            std::string Name;

            GameCertificateReferenceParams() :
                PlayFabBaseModel(),
                GsdkAlias(),
                Name()
            {}

            GameCertificateReferenceParams(const GameCertificateReferenceParams& src) :
                PlayFabBaseModel(),
                GsdkAlias(src.GsdkAlias),
                Name(src.Name)
            {}

            ~GameCertificateReferenceParams() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["GsdkAlias"], GsdkAlias);
                FromJsonUtilS(input["Name"], Name);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_GsdkAlias; ToJsonUtilS(GsdkAlias, each_GsdkAlias); output["GsdkAlias"] = each_GsdkAlias;
                Json::Value each_Name; ToJsonUtilS(Name, each_Name); output["Name"] = each_Name;
                return output;
            }
        };

        struct LinuxInstrumentationConfiguration : public PlayFabBaseModel
        {
            bool IsEnabled;

            LinuxInstrumentationConfiguration() :
                PlayFabBaseModel(),
                IsEnabled()
            {}

            LinuxInstrumentationConfiguration(const LinuxInstrumentationConfiguration& src) :
                PlayFabBaseModel(),
                IsEnabled(src.IsEnabled)
            {}

            ~LinuxInstrumentationConfiguration() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilP(input["IsEnabled"], IsEnabled);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_IsEnabled; ToJsonUtilP(IsEnabled, each_IsEnabled); output["IsEnabled"] = each_IsEnabled;
                return output;
            }
        };

        struct Port : public PlayFabBaseModel
        {
            std::string Name;
            Int32 Num;
            ProtocolType Protocol;

            Port() :
                PlayFabBaseModel(),
                Name(),
                Num(),
                Protocol()
            {}

            Port(const Port& src) :
                PlayFabBaseModel(),
                Name(src.Name),
                Num(src.Num),
                Protocol(src.Protocol)
            {}

            ~Port() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["Name"], Name);
                FromJsonUtilP(input["Num"], Num);
                FromJsonEnum(input["Protocol"], Protocol);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_Name; ToJsonUtilS(Name, each_Name); output["Name"] = each_Name;
                Json::Value each_Num; ToJsonUtilP(Num, each_Num); output["Num"] = each_Num;
                Json::Value each_Protocol; ToJsonEnum(Protocol, each_Protocol); output["Protocol"] = each_Protocol;
                return output;
            }
        };

        struct CreateBuildWithCustomContainerRequest : public PlayFabRequestCommon
        {
            Boxed<bool> AreAssetsReadonly;
            std::string BuildName;
            Boxed<ContainerFlavor> pfContainerFlavor;
            Boxed<ContainerImageReference> pfContainerImageReference;
            std::string ContainerRunCommand;
            std::map<std::string, std::string> CustomTags;
            std::list<AssetReferenceParams> GameAssetReferences;
            std::list<GameCertificateReferenceParams> GameCertificateReferences;
            Boxed<LinuxInstrumentationConfiguration> pfLinuxInstrumentationConfiguration;
            std::map<std::string, std::string> Metadata;
            Int32 MultiplayerServerCountPerVm;
            std::list<Port> Ports;
            std::list<BuildRegionParams> RegionConfigurations;
            Boxed<bool> UseStreamingForAssetDownloads;
            Boxed<AzureVmSize> VmSize;

            CreateBuildWithCustomContainerRequest() :
                PlayFabRequestCommon(),
                AreAssetsReadonly(),
                BuildName(),
                pfContainerFlavor(),
                pfContainerImageReference(),
                ContainerRunCommand(),
                CustomTags(),
                GameAssetReferences(),
                GameCertificateReferences(),
                pfLinuxInstrumentationConfiguration(),
                Metadata(),
                MultiplayerServerCountPerVm(),
                Ports(),
                RegionConfigurations(),
                UseStreamingForAssetDownloads(),
                VmSize()
            {}

            CreateBuildWithCustomContainerRequest(const CreateBuildWithCustomContainerRequest& src) :
                PlayFabRequestCommon(),
                AreAssetsReadonly(src.AreAssetsReadonly),
                BuildName(src.BuildName),
                pfContainerFlavor(src.pfContainerFlavor),
                pfContainerImageReference(src.pfContainerImageReference),
                ContainerRunCommand(src.ContainerRunCommand),
                CustomTags(src.CustomTags),
                GameAssetReferences(src.GameAssetReferences),
                GameCertificateReferences(src.GameCertificateReferences),
                pfLinuxInstrumentationConfiguration(src.pfLinuxInstrumentationConfiguration),
                Metadata(src.Metadata),
                MultiplayerServerCountPerVm(src.MultiplayerServerCountPerVm),
                Ports(src.Ports),
                RegionConfigurations(src.RegionConfigurations),
                UseStreamingForAssetDownloads(src.UseStreamingForAssetDownloads),
                VmSize(src.VmSize)
            {}

            ~CreateBuildWithCustomContainerRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilP(input["AreAssetsReadonly"], AreAssetsReadonly);
                FromJsonUtilS(input["BuildName"], BuildName);
                FromJsonUtilE(input["ContainerFlavor"], pfContainerFlavor);
                FromJsonUtilO(input["ContainerImageReference"], pfContainerImageReference);
                FromJsonUtilS(input["ContainerRunCommand"], ContainerRunCommand);
                FromJsonUtilS(input["CustomTags"], CustomTags);
                FromJsonUtilO(input["GameAssetReferences"], GameAssetReferences);
                FromJsonUtilO(input["GameCertificateReferences"], GameCertificateReferences);
                FromJsonUtilO(input["LinuxInstrumentationConfiguration"], pfLinuxInstrumentationConfiguration);
                FromJsonUtilS(input["Metadata"], Metadata);
                FromJsonUtilP(input["MultiplayerServerCountPerVm"], MultiplayerServerCountPerVm);
                FromJsonUtilO(input["Ports"], Ports);
                FromJsonUtilO(input["RegionConfigurations"], RegionConfigurations);
                FromJsonUtilP(input["UseStreamingForAssetDownloads"], UseStreamingForAssetDownloads);
                FromJsonUtilE(input["VmSize"], VmSize);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_AreAssetsReadonly; ToJsonUtilP(AreAssetsReadonly, each_AreAssetsReadonly); output["AreAssetsReadonly"] = each_AreAssetsReadonly;
                Json::Value each_BuildName; ToJsonUtilS(BuildName, each_BuildName); output["BuildName"] = each_BuildName;
                Json::Value each_pfContainerFlavor; ToJsonUtilE(pfContainerFlavor, each_pfContainerFlavor); output["ContainerFlavor"] = each_pfContainerFlavor;
                Json::Value each_pfContainerImageReference; ToJsonUtilO(pfContainerImageReference, each_pfContainerImageReference); output["ContainerImageReference"] = each_pfContainerImageReference;
                Json::Value each_ContainerRunCommand; ToJsonUtilS(ContainerRunCommand, each_ContainerRunCommand); output["ContainerRunCommand"] = each_ContainerRunCommand;
                Json::Value each_CustomTags; ToJsonUtilS(CustomTags, each_CustomTags); output["CustomTags"] = each_CustomTags;
                Json::Value each_GameAssetReferences; ToJsonUtilO(GameAssetReferences, each_GameAssetReferences); output["GameAssetReferences"] = each_GameAssetReferences;
                Json::Value each_GameCertificateReferences; ToJsonUtilO(GameCertificateReferences, each_GameCertificateReferences); output["GameCertificateReferences"] = each_GameCertificateReferences;
                Json::Value each_pfLinuxInstrumentationConfiguration; ToJsonUtilO(pfLinuxInstrumentationConfiguration, each_pfLinuxInstrumentationConfiguration); output["LinuxInstrumentationConfiguration"] = each_pfLinuxInstrumentationConfiguration;
                Json::Value each_Metadata; ToJsonUtilS(Metadata, each_Metadata); output["Metadata"] = each_Metadata;
                Json::Value each_MultiplayerServerCountPerVm; ToJsonUtilP(MultiplayerServerCountPerVm, each_MultiplayerServerCountPerVm); output["MultiplayerServerCountPerVm"] = each_MultiplayerServerCountPerVm;
                Json::Value each_Ports; ToJsonUtilO(Ports, each_Ports); output["Ports"] = each_Ports;
                Json::Value each_RegionConfigurations; ToJsonUtilO(RegionConfigurations, each_RegionConfigurations); output["RegionConfigurations"] = each_RegionConfigurations;
                Json::Value each_UseStreamingForAssetDownloads; ToJsonUtilP(UseStreamingForAssetDownloads, each_UseStreamingForAssetDownloads); output["UseStreamingForAssetDownloads"] = each_UseStreamingForAssetDownloads;
                Json::Value each_VmSize; ToJsonUtilE(VmSize, each_VmSize); output["VmSize"] = each_VmSize;
                return output;
            }
        };

        struct GameCertificateReference : public PlayFabBaseModel
        {
            std::string GsdkAlias;
            std::string Name;

            GameCertificateReference() :
                PlayFabBaseModel(),
                GsdkAlias(),
                Name()
            {}

            GameCertificateReference(const GameCertificateReference& src) :
                PlayFabBaseModel(),
                GsdkAlias(src.GsdkAlias),
                Name(src.Name)
            {}

            ~GameCertificateReference() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["GsdkAlias"], GsdkAlias);
                FromJsonUtilS(input["Name"], Name);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_GsdkAlias; ToJsonUtilS(GsdkAlias, each_GsdkAlias); output["GsdkAlias"] = each_GsdkAlias;
                Json::Value each_Name; ToJsonUtilS(Name, each_Name); output["Name"] = each_Name;
                return output;
            }
        };

        struct CreateBuildWithCustomContainerResponse : public PlayFabResultCommon
        {
            Boxed<bool> AreAssetsReadonly;
            std::string BuildId;
            std::string BuildName;
            Boxed<ContainerFlavor> pfContainerFlavor;
            std::string ContainerRunCommand;
            Boxed<time_t> CreationTime;
            Boxed<ContainerImageReference> CustomGameContainerImage;
            std::list<AssetReference> GameAssetReferences;
            std::list<GameCertificateReference> GameCertificateReferences;
            Boxed<LinuxInstrumentationConfiguration> pfLinuxInstrumentationConfiguration;
            std::map<std::string, std::string> Metadata;
            Int32 MultiplayerServerCountPerVm;
            std::string OsPlatform;
            std::list<Port> Ports;
            std::list<BuildRegion> RegionConfigurations;
            std::string ServerType;
            Boxed<bool> UseStreamingForAssetDownloads;
            Boxed<AzureVmSize> VmSize;

            CreateBuildWithCustomContainerResponse() :
                PlayFabResultCommon(),
                AreAssetsReadonly(),
                BuildId(),
                BuildName(),
                pfContainerFlavor(),
                ContainerRunCommand(),
                CreationTime(),
                CustomGameContainerImage(),
                GameAssetReferences(),
                GameCertificateReferences(),
                pfLinuxInstrumentationConfiguration(),
                Metadata(),
                MultiplayerServerCountPerVm(),
                OsPlatform(),
                Ports(),
                RegionConfigurations(),
                ServerType(),
                UseStreamingForAssetDownloads(),
                VmSize()
            {}

            CreateBuildWithCustomContainerResponse(const CreateBuildWithCustomContainerResponse& src) :
                PlayFabResultCommon(),
                AreAssetsReadonly(src.AreAssetsReadonly),
                BuildId(src.BuildId),
                BuildName(src.BuildName),
                pfContainerFlavor(src.pfContainerFlavor),
                ContainerRunCommand(src.ContainerRunCommand),
                CreationTime(src.CreationTime),
                CustomGameContainerImage(src.CustomGameContainerImage),
                GameAssetReferences(src.GameAssetReferences),
                GameCertificateReferences(src.GameCertificateReferences),
                pfLinuxInstrumentationConfiguration(src.pfLinuxInstrumentationConfiguration),
                Metadata(src.Metadata),
                MultiplayerServerCountPerVm(src.MultiplayerServerCountPerVm),
                OsPlatform(src.OsPlatform),
                Ports(src.Ports),
                RegionConfigurations(src.RegionConfigurations),
                ServerType(src.ServerType),
                UseStreamingForAssetDownloads(src.UseStreamingForAssetDownloads),
                VmSize(src.VmSize)
            {}

            ~CreateBuildWithCustomContainerResponse() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilP(input["AreAssetsReadonly"], AreAssetsReadonly);
                FromJsonUtilS(input["BuildId"], BuildId);
                FromJsonUtilS(input["BuildName"], BuildName);
                FromJsonUtilE(input["ContainerFlavor"], pfContainerFlavor);
                FromJsonUtilS(input["ContainerRunCommand"], ContainerRunCommand);
                FromJsonUtilT(input["CreationTime"], CreationTime);
                FromJsonUtilO(input["CustomGameContainerImage"], CustomGameContainerImage);
                FromJsonUtilO(input["GameAssetReferences"], GameAssetReferences);
                FromJsonUtilO(input["GameCertificateReferences"], GameCertificateReferences);
                FromJsonUtilO(input["LinuxInstrumentationConfiguration"], pfLinuxInstrumentationConfiguration);
                FromJsonUtilS(input["Metadata"], Metadata);
                FromJsonUtilP(input["MultiplayerServerCountPerVm"], MultiplayerServerCountPerVm);
                FromJsonUtilS(input["OsPlatform"], OsPlatform);
                FromJsonUtilO(input["Ports"], Ports);
                FromJsonUtilO(input["RegionConfigurations"], RegionConfigurations);
                FromJsonUtilS(input["ServerType"], ServerType);
                FromJsonUtilP(input["UseStreamingForAssetDownloads"], UseStreamingForAssetDownloads);
                FromJsonUtilE(input["VmSize"], VmSize);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_AreAssetsReadonly; ToJsonUtilP(AreAssetsReadonly, each_AreAssetsReadonly); output["AreAssetsReadonly"] = each_AreAssetsReadonly;
                Json::Value each_BuildId; ToJsonUtilS(BuildId, each_BuildId); output["BuildId"] = each_BuildId;
                Json::Value each_BuildName; ToJsonUtilS(BuildName, each_BuildName); output["BuildName"] = each_BuildName;
                Json::Value each_pfContainerFlavor; ToJsonUtilE(pfContainerFlavor, each_pfContainerFlavor); output["ContainerFlavor"] = each_pfContainerFlavor;
                Json::Value each_ContainerRunCommand; ToJsonUtilS(ContainerRunCommand, each_ContainerRunCommand); output["ContainerRunCommand"] = each_ContainerRunCommand;
                Json::Value each_CreationTime; ToJsonUtilT(CreationTime, each_CreationTime); output["CreationTime"] = each_CreationTime;
                Json::Value each_CustomGameContainerImage; ToJsonUtilO(CustomGameContainerImage, each_CustomGameContainerImage); output["CustomGameContainerImage"] = each_CustomGameContainerImage;
                Json::Value each_GameAssetReferences; ToJsonUtilO(GameAssetReferences, each_GameAssetReferences); output["GameAssetReferences"] = each_GameAssetReferences;
                Json::Value each_GameCertificateReferences; ToJsonUtilO(GameCertificateReferences, each_GameCertificateReferences); output["GameCertificateReferences"] = each_GameCertificateReferences;
                Json::Value each_pfLinuxInstrumentationConfiguration; ToJsonUtilO(pfLinuxInstrumentationConfiguration, each_pfLinuxInstrumentationConfiguration); output["LinuxInstrumentationConfiguration"] = each_pfLinuxInstrumentationConfiguration;
                Json::Value each_Metadata; ToJsonUtilS(Metadata, each_Metadata); output["Metadata"] = each_Metadata;
                Json::Value each_MultiplayerServerCountPerVm; ToJsonUtilP(MultiplayerServerCountPerVm, each_MultiplayerServerCountPerVm); output["MultiplayerServerCountPerVm"] = each_MultiplayerServerCountPerVm;
                Json::Value each_OsPlatform; ToJsonUtilS(OsPlatform, each_OsPlatform); output["OsPlatform"] = each_OsPlatform;
                Json::Value each_Ports; ToJsonUtilO(Ports, each_Ports); output["Ports"] = each_Ports;
                Json::Value each_RegionConfigurations; ToJsonUtilO(RegionConfigurations, each_RegionConfigurations); output["RegionConfigurations"] = each_RegionConfigurations;
                Json::Value each_ServerType; ToJsonUtilS(ServerType, each_ServerType); output["ServerType"] = each_ServerType;
                Json::Value each_UseStreamingForAssetDownloads; ToJsonUtilP(UseStreamingForAssetDownloads, each_UseStreamingForAssetDownloads); output["UseStreamingForAssetDownloads"] = each_UseStreamingForAssetDownloads;
                Json::Value each_VmSize; ToJsonUtilE(VmSize, each_VmSize); output["VmSize"] = each_VmSize;
                return output;
            }
        };

        struct InstrumentationConfiguration : public PlayFabBaseModel
        {
            Boxed<bool> IsEnabled;
            std::list<std::string> ProcessesToMonitor;

            InstrumentationConfiguration() :
                PlayFabBaseModel(),
                IsEnabled(),
                ProcessesToMonitor()
            {}

            InstrumentationConfiguration(const InstrumentationConfiguration& src) :
                PlayFabBaseModel(),
                IsEnabled(src.IsEnabled),
                ProcessesToMonitor(src.ProcessesToMonitor)
            {}

            ~InstrumentationConfiguration() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilP(input["IsEnabled"], IsEnabled);
                FromJsonUtilS(input["ProcessesToMonitor"], ProcessesToMonitor);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_IsEnabled; ToJsonUtilP(IsEnabled, each_IsEnabled); output["IsEnabled"] = each_IsEnabled;
                Json::Value each_ProcessesToMonitor; ToJsonUtilS(ProcessesToMonitor, each_ProcessesToMonitor); output["ProcessesToMonitor"] = each_ProcessesToMonitor;
                return output;
            }
        };

        struct CreateBuildWithManagedContainerRequest : public PlayFabRequestCommon
        {
            Boxed<bool> AreAssetsReadonly;
            std::string BuildName;
            Boxed<ContainerFlavor> pfContainerFlavor;
            std::map<std::string, std::string> CustomTags;
            std::list<AssetReferenceParams> GameAssetReferences;
            std::list<GameCertificateReferenceParams> GameCertificateReferences;
            std::string GameWorkingDirectory;
            Boxed<InstrumentationConfiguration> pfInstrumentationConfiguration;
            std::map<std::string, std::string> Metadata;
            Int32 MultiplayerServerCountPerVm;
            std::list<Port> Ports;
            std::list<BuildRegionParams> RegionConfigurations;
            std::string StartMultiplayerServerCommand;
            Boxed<bool> UseStreamingForAssetDownloads;
            Boxed<AzureVmSize> VmSize;

            CreateBuildWithManagedContainerRequest() :
                PlayFabRequestCommon(),
                AreAssetsReadonly(),
                BuildName(),
                pfContainerFlavor(),
                CustomTags(),
                GameAssetReferences(),
                GameCertificateReferences(),
                GameWorkingDirectory(),
                pfInstrumentationConfiguration(),
                Metadata(),
                MultiplayerServerCountPerVm(),
                Ports(),
                RegionConfigurations(),
                StartMultiplayerServerCommand(),
                UseStreamingForAssetDownloads(),
                VmSize()
            {}

            CreateBuildWithManagedContainerRequest(const CreateBuildWithManagedContainerRequest& src) :
                PlayFabRequestCommon(),
                AreAssetsReadonly(src.AreAssetsReadonly),
                BuildName(src.BuildName),
                pfContainerFlavor(src.pfContainerFlavor),
                CustomTags(src.CustomTags),
                GameAssetReferences(src.GameAssetReferences),
                GameCertificateReferences(src.GameCertificateReferences),
                GameWorkingDirectory(src.GameWorkingDirectory),
                pfInstrumentationConfiguration(src.pfInstrumentationConfiguration),
                Metadata(src.Metadata),
                MultiplayerServerCountPerVm(src.MultiplayerServerCountPerVm),
                Ports(src.Ports),
                RegionConfigurations(src.RegionConfigurations),
                StartMultiplayerServerCommand(src.StartMultiplayerServerCommand),
                UseStreamingForAssetDownloads(src.UseStreamingForAssetDownloads),
                VmSize(src.VmSize)
            {}

            ~CreateBuildWithManagedContainerRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilP(input["AreAssetsReadonly"], AreAssetsReadonly);
                FromJsonUtilS(input["BuildName"], BuildName);
                FromJsonUtilE(input["ContainerFlavor"], pfContainerFlavor);
                FromJsonUtilS(input["CustomTags"], CustomTags);
                FromJsonUtilO(input["GameAssetReferences"], GameAssetReferences);
                FromJsonUtilO(input["GameCertificateReferences"], GameCertificateReferences);
                FromJsonUtilS(input["GameWorkingDirectory"], GameWorkingDirectory);
                FromJsonUtilO(input["InstrumentationConfiguration"], pfInstrumentationConfiguration);
                FromJsonUtilS(input["Metadata"], Metadata);
                FromJsonUtilP(input["MultiplayerServerCountPerVm"], MultiplayerServerCountPerVm);
                FromJsonUtilO(input["Ports"], Ports);
                FromJsonUtilO(input["RegionConfigurations"], RegionConfigurations);
                FromJsonUtilS(input["StartMultiplayerServerCommand"], StartMultiplayerServerCommand);
                FromJsonUtilP(input["UseStreamingForAssetDownloads"], UseStreamingForAssetDownloads);
                FromJsonUtilE(input["VmSize"], VmSize);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_AreAssetsReadonly; ToJsonUtilP(AreAssetsReadonly, each_AreAssetsReadonly); output["AreAssetsReadonly"] = each_AreAssetsReadonly;
                Json::Value each_BuildName; ToJsonUtilS(BuildName, each_BuildName); output["BuildName"] = each_BuildName;
                Json::Value each_pfContainerFlavor; ToJsonUtilE(pfContainerFlavor, each_pfContainerFlavor); output["ContainerFlavor"] = each_pfContainerFlavor;
                Json::Value each_CustomTags; ToJsonUtilS(CustomTags, each_CustomTags); output["CustomTags"] = each_CustomTags;
                Json::Value each_GameAssetReferences; ToJsonUtilO(GameAssetReferences, each_GameAssetReferences); output["GameAssetReferences"] = each_GameAssetReferences;
                Json::Value each_GameCertificateReferences; ToJsonUtilO(GameCertificateReferences, each_GameCertificateReferences); output["GameCertificateReferences"] = each_GameCertificateReferences;
                Json::Value each_GameWorkingDirectory; ToJsonUtilS(GameWorkingDirectory, each_GameWorkingDirectory); output["GameWorkingDirectory"] = each_GameWorkingDirectory;
                Json::Value each_pfInstrumentationConfiguration; ToJsonUtilO(pfInstrumentationConfiguration, each_pfInstrumentationConfiguration); output["InstrumentationConfiguration"] = each_pfInstrumentationConfiguration;
                Json::Value each_Metadata; ToJsonUtilS(Metadata, each_Metadata); output["Metadata"] = each_Metadata;
                Json::Value each_MultiplayerServerCountPerVm; ToJsonUtilP(MultiplayerServerCountPerVm, each_MultiplayerServerCountPerVm); output["MultiplayerServerCountPerVm"] = each_MultiplayerServerCountPerVm;
                Json::Value each_Ports; ToJsonUtilO(Ports, each_Ports); output["Ports"] = each_Ports;
                Json::Value each_RegionConfigurations; ToJsonUtilO(RegionConfigurations, each_RegionConfigurations); output["RegionConfigurations"] = each_RegionConfigurations;
                Json::Value each_StartMultiplayerServerCommand; ToJsonUtilS(StartMultiplayerServerCommand, each_StartMultiplayerServerCommand); output["StartMultiplayerServerCommand"] = each_StartMultiplayerServerCommand;
                Json::Value each_UseStreamingForAssetDownloads; ToJsonUtilP(UseStreamingForAssetDownloads, each_UseStreamingForAssetDownloads); output["UseStreamingForAssetDownloads"] = each_UseStreamingForAssetDownloads;
                Json::Value each_VmSize; ToJsonUtilE(VmSize, each_VmSize); output["VmSize"] = each_VmSize;
                return output;
            }
        };

        struct CreateBuildWithManagedContainerResponse : public PlayFabResultCommon
        {
            Boxed<bool> AreAssetsReadonly;
            std::string BuildId;
            std::string BuildName;
            Boxed<ContainerFlavor> pfContainerFlavor;
            Boxed<time_t> CreationTime;
            std::list<AssetReference> GameAssetReferences;
            std::list<GameCertificateReference> GameCertificateReferences;
            std::string GameWorkingDirectory;
            Boxed<InstrumentationConfiguration> pfInstrumentationConfiguration;
            std::map<std::string, std::string> Metadata;
            Int32 MultiplayerServerCountPerVm;
            std::string OsPlatform;
            std::list<Port> Ports;
            std::list<BuildRegion> RegionConfigurations;
            std::string ServerType;
            std::string StartMultiplayerServerCommand;
            Boxed<bool> UseStreamingForAssetDownloads;
            Boxed<AzureVmSize> VmSize;

            CreateBuildWithManagedContainerResponse() :
                PlayFabResultCommon(),
                AreAssetsReadonly(),
                BuildId(),
                BuildName(),
                pfContainerFlavor(),
                CreationTime(),
                GameAssetReferences(),
                GameCertificateReferences(),
                GameWorkingDirectory(),
                pfInstrumentationConfiguration(),
                Metadata(),
                MultiplayerServerCountPerVm(),
                OsPlatform(),
                Ports(),
                RegionConfigurations(),
                ServerType(),
                StartMultiplayerServerCommand(),
                UseStreamingForAssetDownloads(),
                VmSize()
            {}

            CreateBuildWithManagedContainerResponse(const CreateBuildWithManagedContainerResponse& src) :
                PlayFabResultCommon(),
                AreAssetsReadonly(src.AreAssetsReadonly),
                BuildId(src.BuildId),
                BuildName(src.BuildName),
                pfContainerFlavor(src.pfContainerFlavor),
                CreationTime(src.CreationTime),
                GameAssetReferences(src.GameAssetReferences),
                GameCertificateReferences(src.GameCertificateReferences),
                GameWorkingDirectory(src.GameWorkingDirectory),
                pfInstrumentationConfiguration(src.pfInstrumentationConfiguration),
                Metadata(src.Metadata),
                MultiplayerServerCountPerVm(src.MultiplayerServerCountPerVm),
                OsPlatform(src.OsPlatform),
                Ports(src.Ports),
                RegionConfigurations(src.RegionConfigurations),
                ServerType(src.ServerType),
                StartMultiplayerServerCommand(src.StartMultiplayerServerCommand),
                UseStreamingForAssetDownloads(src.UseStreamingForAssetDownloads),
                VmSize(src.VmSize)
            {}

            ~CreateBuildWithManagedContainerResponse() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilP(input["AreAssetsReadonly"], AreAssetsReadonly);
                FromJsonUtilS(input["BuildId"], BuildId);
                FromJsonUtilS(input["BuildName"], BuildName);
                FromJsonUtilE(input["ContainerFlavor"], pfContainerFlavor);
                FromJsonUtilT(input["CreationTime"], CreationTime);
                FromJsonUtilO(input["GameAssetReferences"], GameAssetReferences);
                FromJsonUtilO(input["GameCertificateReferences"], GameCertificateReferences);
                FromJsonUtilS(input["GameWorkingDirectory"], GameWorkingDirectory);
                FromJsonUtilO(input["InstrumentationConfiguration"], pfInstrumentationConfiguration);
                FromJsonUtilS(input["Metadata"], Metadata);
                FromJsonUtilP(input["MultiplayerServerCountPerVm"], MultiplayerServerCountPerVm);
                FromJsonUtilS(input["OsPlatform"], OsPlatform);
                FromJsonUtilO(input["Ports"], Ports);
                FromJsonUtilO(input["RegionConfigurations"], RegionConfigurations);
                FromJsonUtilS(input["ServerType"], ServerType);
                FromJsonUtilS(input["StartMultiplayerServerCommand"], StartMultiplayerServerCommand);
                FromJsonUtilP(input["UseStreamingForAssetDownloads"], UseStreamingForAssetDownloads);
                FromJsonUtilE(input["VmSize"], VmSize);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_AreAssetsReadonly; ToJsonUtilP(AreAssetsReadonly, each_AreAssetsReadonly); output["AreAssetsReadonly"] = each_AreAssetsReadonly;
                Json::Value each_BuildId; ToJsonUtilS(BuildId, each_BuildId); output["BuildId"] = each_BuildId;
                Json::Value each_BuildName; ToJsonUtilS(BuildName, each_BuildName); output["BuildName"] = each_BuildName;
                Json::Value each_pfContainerFlavor; ToJsonUtilE(pfContainerFlavor, each_pfContainerFlavor); output["ContainerFlavor"] = each_pfContainerFlavor;
                Json::Value each_CreationTime; ToJsonUtilT(CreationTime, each_CreationTime); output["CreationTime"] = each_CreationTime;
                Json::Value each_GameAssetReferences; ToJsonUtilO(GameAssetReferences, each_GameAssetReferences); output["GameAssetReferences"] = each_GameAssetReferences;
                Json::Value each_GameCertificateReferences; ToJsonUtilO(GameCertificateReferences, each_GameCertificateReferences); output["GameCertificateReferences"] = each_GameCertificateReferences;
                Json::Value each_GameWorkingDirectory; ToJsonUtilS(GameWorkingDirectory, each_GameWorkingDirectory); output["GameWorkingDirectory"] = each_GameWorkingDirectory;
                Json::Value each_pfInstrumentationConfiguration; ToJsonUtilO(pfInstrumentationConfiguration, each_pfInstrumentationConfiguration); output["InstrumentationConfiguration"] = each_pfInstrumentationConfiguration;
                Json::Value each_Metadata; ToJsonUtilS(Metadata, each_Metadata); output["Metadata"] = each_Metadata;
                Json::Value each_MultiplayerServerCountPerVm; ToJsonUtilP(MultiplayerServerCountPerVm, each_MultiplayerServerCountPerVm); output["MultiplayerServerCountPerVm"] = each_MultiplayerServerCountPerVm;
                Json::Value each_OsPlatform; ToJsonUtilS(OsPlatform, each_OsPlatform); output["OsPlatform"] = each_OsPlatform;
                Json::Value each_Ports; ToJsonUtilO(Ports, each_Ports); output["Ports"] = each_Ports;
                Json::Value each_RegionConfigurations; ToJsonUtilO(RegionConfigurations, each_RegionConfigurations); output["RegionConfigurations"] = each_RegionConfigurations;
                Json::Value each_ServerType; ToJsonUtilS(ServerType, each_ServerType); output["ServerType"] = each_ServerType;
                Json::Value each_StartMultiplayerServerCommand; ToJsonUtilS(StartMultiplayerServerCommand, each_StartMultiplayerServerCommand); output["StartMultiplayerServerCommand"] = each_StartMultiplayerServerCommand;
                Json::Value each_UseStreamingForAssetDownloads; ToJsonUtilP(UseStreamingForAssetDownloads, each_UseStreamingForAssetDownloads); output["UseStreamingForAssetDownloads"] = each_UseStreamingForAssetDownloads;
                Json::Value each_VmSize; ToJsonUtilE(VmSize, each_VmSize); output["VmSize"] = each_VmSize;
                return output;
            }
        };

        struct CreateBuildWithProcessBasedServerRequest : public PlayFabRequestCommon
        {
            Boxed<bool> AreAssetsReadonly;
            std::string BuildName;
            std::map<std::string, std::string> CustomTags;
            std::list<AssetReferenceParams> GameAssetReferences;
            std::list<GameCertificateReferenceParams> GameCertificateReferences;
            std::string GameWorkingDirectory;
            Boxed<InstrumentationConfiguration> pfInstrumentationConfiguration;
            Boxed<bool> IsOSPreview;
            std::map<std::string, std::string> Metadata;
            Int32 MultiplayerServerCountPerVm;
            std::string OsPlatform;
            std::list<Port> Ports;
            std::list<BuildRegionParams> RegionConfigurations;
            std::string StartMultiplayerServerCommand;
            Boxed<bool> UseStreamingForAssetDownloads;
            Boxed<AzureVmSize> VmSize;

            CreateBuildWithProcessBasedServerRequest() :
                PlayFabRequestCommon(),
                AreAssetsReadonly(),
                BuildName(),
                CustomTags(),
                GameAssetReferences(),
                GameCertificateReferences(),
                GameWorkingDirectory(),
                pfInstrumentationConfiguration(),
                IsOSPreview(),
                Metadata(),
                MultiplayerServerCountPerVm(),
                OsPlatform(),
                Ports(),
                RegionConfigurations(),
                StartMultiplayerServerCommand(),
                UseStreamingForAssetDownloads(),
                VmSize()
            {}

            CreateBuildWithProcessBasedServerRequest(const CreateBuildWithProcessBasedServerRequest& src) :
                PlayFabRequestCommon(),
                AreAssetsReadonly(src.AreAssetsReadonly),
                BuildName(src.BuildName),
                CustomTags(src.CustomTags),
                GameAssetReferences(src.GameAssetReferences),
                GameCertificateReferences(src.GameCertificateReferences),
                GameWorkingDirectory(src.GameWorkingDirectory),
                pfInstrumentationConfiguration(src.pfInstrumentationConfiguration),
                IsOSPreview(src.IsOSPreview),
                Metadata(src.Metadata),
                MultiplayerServerCountPerVm(src.MultiplayerServerCountPerVm),
                OsPlatform(src.OsPlatform),
                Ports(src.Ports),
                RegionConfigurations(src.RegionConfigurations),
                StartMultiplayerServerCommand(src.StartMultiplayerServerCommand),
                UseStreamingForAssetDownloads(src.UseStreamingForAssetDownloads),
                VmSize(src.VmSize)
            {}

            ~CreateBuildWithProcessBasedServerRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilP(input["AreAssetsReadonly"], AreAssetsReadonly);
                FromJsonUtilS(input["BuildName"], BuildName);
                FromJsonUtilS(input["CustomTags"], CustomTags);
                FromJsonUtilO(input["GameAssetReferences"], GameAssetReferences);
                FromJsonUtilO(input["GameCertificateReferences"], GameCertificateReferences);
                FromJsonUtilS(input["GameWorkingDirectory"], GameWorkingDirectory);
                FromJsonUtilO(input["InstrumentationConfiguration"], pfInstrumentationConfiguration);
                FromJsonUtilP(input["IsOSPreview"], IsOSPreview);
                FromJsonUtilS(input["Metadata"], Metadata);
                FromJsonUtilP(input["MultiplayerServerCountPerVm"], MultiplayerServerCountPerVm);
                FromJsonUtilS(input["OsPlatform"], OsPlatform);
                FromJsonUtilO(input["Ports"], Ports);
                FromJsonUtilO(input["RegionConfigurations"], RegionConfigurations);
                FromJsonUtilS(input["StartMultiplayerServerCommand"], StartMultiplayerServerCommand);
                FromJsonUtilP(input["UseStreamingForAssetDownloads"], UseStreamingForAssetDownloads);
                FromJsonUtilE(input["VmSize"], VmSize);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_AreAssetsReadonly; ToJsonUtilP(AreAssetsReadonly, each_AreAssetsReadonly); output["AreAssetsReadonly"] = each_AreAssetsReadonly;
                Json::Value each_BuildName; ToJsonUtilS(BuildName, each_BuildName); output["BuildName"] = each_BuildName;
                Json::Value each_CustomTags; ToJsonUtilS(CustomTags, each_CustomTags); output["CustomTags"] = each_CustomTags;
                Json::Value each_GameAssetReferences; ToJsonUtilO(GameAssetReferences, each_GameAssetReferences); output["GameAssetReferences"] = each_GameAssetReferences;
                Json::Value each_GameCertificateReferences; ToJsonUtilO(GameCertificateReferences, each_GameCertificateReferences); output["GameCertificateReferences"] = each_GameCertificateReferences;
                Json::Value each_GameWorkingDirectory; ToJsonUtilS(GameWorkingDirectory, each_GameWorkingDirectory); output["GameWorkingDirectory"] = each_GameWorkingDirectory;
                Json::Value each_pfInstrumentationConfiguration; ToJsonUtilO(pfInstrumentationConfiguration, each_pfInstrumentationConfiguration); output["InstrumentationConfiguration"] = each_pfInstrumentationConfiguration;
                Json::Value each_IsOSPreview; ToJsonUtilP(IsOSPreview, each_IsOSPreview); output["IsOSPreview"] = each_IsOSPreview;
                Json::Value each_Metadata; ToJsonUtilS(Metadata, each_Metadata); output["Metadata"] = each_Metadata;
                Json::Value each_MultiplayerServerCountPerVm; ToJsonUtilP(MultiplayerServerCountPerVm, each_MultiplayerServerCountPerVm); output["MultiplayerServerCountPerVm"] = each_MultiplayerServerCountPerVm;
                Json::Value each_OsPlatform; ToJsonUtilS(OsPlatform, each_OsPlatform); output["OsPlatform"] = each_OsPlatform;
                Json::Value each_Ports; ToJsonUtilO(Ports, each_Ports); output["Ports"] = each_Ports;
                Json::Value each_RegionConfigurations; ToJsonUtilO(RegionConfigurations, each_RegionConfigurations); output["RegionConfigurations"] = each_RegionConfigurations;
                Json::Value each_StartMultiplayerServerCommand; ToJsonUtilS(StartMultiplayerServerCommand, each_StartMultiplayerServerCommand); output["StartMultiplayerServerCommand"] = each_StartMultiplayerServerCommand;
                Json::Value each_UseStreamingForAssetDownloads; ToJsonUtilP(UseStreamingForAssetDownloads, each_UseStreamingForAssetDownloads); output["UseStreamingForAssetDownloads"] = each_UseStreamingForAssetDownloads;
                Json::Value each_VmSize; ToJsonUtilE(VmSize, each_VmSize); output["VmSize"] = each_VmSize;
                return output;
            }
        };

        struct CreateBuildWithProcessBasedServerResponse : public PlayFabResultCommon
        {
            Boxed<bool> AreAssetsReadonly;
            std::string BuildId;
            std::string BuildName;
            Boxed<ContainerFlavor> pfContainerFlavor;
            Boxed<time_t> CreationTime;
            std::list<AssetReference> GameAssetReferences;
            std::list<GameCertificateReference> GameCertificateReferences;
            std::string GameWorkingDirectory;
            Boxed<InstrumentationConfiguration> pfInstrumentationConfiguration;
            Boxed<bool> IsOSPreview;
            std::map<std::string, std::string> Metadata;
            Int32 MultiplayerServerCountPerVm;
            std::string OsPlatform;
            std::list<Port> Ports;
            std::list<BuildRegion> RegionConfigurations;
            std::string ServerType;
            std::string StartMultiplayerServerCommand;
            Boxed<bool> UseStreamingForAssetDownloads;
            Boxed<AzureVmSize> VmSize;

            CreateBuildWithProcessBasedServerResponse() :
                PlayFabResultCommon(),
                AreAssetsReadonly(),
                BuildId(),
                BuildName(),
                pfContainerFlavor(),
                CreationTime(),
                GameAssetReferences(),
                GameCertificateReferences(),
                GameWorkingDirectory(),
                pfInstrumentationConfiguration(),
                IsOSPreview(),
                Metadata(),
                MultiplayerServerCountPerVm(),
                OsPlatform(),
                Ports(),
                RegionConfigurations(),
                ServerType(),
                StartMultiplayerServerCommand(),
                UseStreamingForAssetDownloads(),
                VmSize()
            {}

            CreateBuildWithProcessBasedServerResponse(const CreateBuildWithProcessBasedServerResponse& src) :
                PlayFabResultCommon(),
                AreAssetsReadonly(src.AreAssetsReadonly),
                BuildId(src.BuildId),
                BuildName(src.BuildName),
                pfContainerFlavor(src.pfContainerFlavor),
                CreationTime(src.CreationTime),
                GameAssetReferences(src.GameAssetReferences),
                GameCertificateReferences(src.GameCertificateReferences),
                GameWorkingDirectory(src.GameWorkingDirectory),
                pfInstrumentationConfiguration(src.pfInstrumentationConfiguration),
                IsOSPreview(src.IsOSPreview),
                Metadata(src.Metadata),
                MultiplayerServerCountPerVm(src.MultiplayerServerCountPerVm),
                OsPlatform(src.OsPlatform),
                Ports(src.Ports),
                RegionConfigurations(src.RegionConfigurations),
                ServerType(src.ServerType),
                StartMultiplayerServerCommand(src.StartMultiplayerServerCommand),
                UseStreamingForAssetDownloads(src.UseStreamingForAssetDownloads),
                VmSize(src.VmSize)
            {}

            ~CreateBuildWithProcessBasedServerResponse() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilP(input["AreAssetsReadonly"], AreAssetsReadonly);
                FromJsonUtilS(input["BuildId"], BuildId);
                FromJsonUtilS(input["BuildName"], BuildName);
                FromJsonUtilE(input["ContainerFlavor"], pfContainerFlavor);
                FromJsonUtilT(input["CreationTime"], CreationTime);
                FromJsonUtilO(input["GameAssetReferences"], GameAssetReferences);
                FromJsonUtilO(input["GameCertificateReferences"], GameCertificateReferences);
                FromJsonUtilS(input["GameWorkingDirectory"], GameWorkingDirectory);
                FromJsonUtilO(input["InstrumentationConfiguration"], pfInstrumentationConfiguration);
                FromJsonUtilP(input["IsOSPreview"], IsOSPreview);
                FromJsonUtilS(input["Metadata"], Metadata);
                FromJsonUtilP(input["MultiplayerServerCountPerVm"], MultiplayerServerCountPerVm);
                FromJsonUtilS(input["OsPlatform"], OsPlatform);
                FromJsonUtilO(input["Ports"], Ports);
                FromJsonUtilO(input["RegionConfigurations"], RegionConfigurations);
                FromJsonUtilS(input["ServerType"], ServerType);
                FromJsonUtilS(input["StartMultiplayerServerCommand"], StartMultiplayerServerCommand);
                FromJsonUtilP(input["UseStreamingForAssetDownloads"], UseStreamingForAssetDownloads);
                FromJsonUtilE(input["VmSize"], VmSize);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_AreAssetsReadonly; ToJsonUtilP(AreAssetsReadonly, each_AreAssetsReadonly); output["AreAssetsReadonly"] = each_AreAssetsReadonly;
                Json::Value each_BuildId; ToJsonUtilS(BuildId, each_BuildId); output["BuildId"] = each_BuildId;
                Json::Value each_BuildName; ToJsonUtilS(BuildName, each_BuildName); output["BuildName"] = each_BuildName;
                Json::Value each_pfContainerFlavor; ToJsonUtilE(pfContainerFlavor, each_pfContainerFlavor); output["ContainerFlavor"] = each_pfContainerFlavor;
                Json::Value each_CreationTime; ToJsonUtilT(CreationTime, each_CreationTime); output["CreationTime"] = each_CreationTime;
                Json::Value each_GameAssetReferences; ToJsonUtilO(GameAssetReferences, each_GameAssetReferences); output["GameAssetReferences"] = each_GameAssetReferences;
                Json::Value each_GameCertificateReferences; ToJsonUtilO(GameCertificateReferences, each_GameCertificateReferences); output["GameCertificateReferences"] = each_GameCertificateReferences;
                Json::Value each_GameWorkingDirectory; ToJsonUtilS(GameWorkingDirectory, each_GameWorkingDirectory); output["GameWorkingDirectory"] = each_GameWorkingDirectory;
                Json::Value each_pfInstrumentationConfiguration; ToJsonUtilO(pfInstrumentationConfiguration, each_pfInstrumentationConfiguration); output["InstrumentationConfiguration"] = each_pfInstrumentationConfiguration;
                Json::Value each_IsOSPreview; ToJsonUtilP(IsOSPreview, each_IsOSPreview); output["IsOSPreview"] = each_IsOSPreview;
                Json::Value each_Metadata; ToJsonUtilS(Metadata, each_Metadata); output["Metadata"] = each_Metadata;
                Json::Value each_MultiplayerServerCountPerVm; ToJsonUtilP(MultiplayerServerCountPerVm, each_MultiplayerServerCountPerVm); output["MultiplayerServerCountPerVm"] = each_MultiplayerServerCountPerVm;
                Json::Value each_OsPlatform; ToJsonUtilS(OsPlatform, each_OsPlatform); output["OsPlatform"] = each_OsPlatform;
                Json::Value each_Ports; ToJsonUtilO(Ports, each_Ports); output["Ports"] = each_Ports;
                Json::Value each_RegionConfigurations; ToJsonUtilO(RegionConfigurations, each_RegionConfigurations); output["RegionConfigurations"] = each_RegionConfigurations;
                Json::Value each_ServerType; ToJsonUtilS(ServerType, each_ServerType); output["ServerType"] = each_ServerType;
                Json::Value each_StartMultiplayerServerCommand; ToJsonUtilS(StartMultiplayerServerCommand, each_StartMultiplayerServerCommand); output["StartMultiplayerServerCommand"] = each_StartMultiplayerServerCommand;
                Json::Value each_UseStreamingForAssetDownloads; ToJsonUtilP(UseStreamingForAssetDownloads, each_UseStreamingForAssetDownloads); output["UseStreamingForAssetDownloads"] = each_UseStreamingForAssetDownloads;
                Json::Value each_VmSize; ToJsonUtilE(VmSize, each_VmSize); output["VmSize"] = each_VmSize;
                return output;
            }
        };

        struct MatchmakingPlayerAttributes : public PlayFabBaseModel
        {
            Json::Value DataObject;
            std::string EscapedDataObject;

            MatchmakingPlayerAttributes() :
                PlayFabBaseModel(),
                DataObject(),
                EscapedDataObject()
            {}

            MatchmakingPlayerAttributes(const MatchmakingPlayerAttributes& src) :
                PlayFabBaseModel(),
                DataObject(src.DataObject),
                EscapedDataObject(src.EscapedDataObject)
            {}

            ~MatchmakingPlayerAttributes() = default;

            void FromJson(const Json::Value& input) override
            {
                DataObject = input["DataObject"];
                FromJsonUtilS(input["EscapedDataObject"], EscapedDataObject);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                output["DataObject"] = DataObject;
                Json::Value each_EscapedDataObject; ToJsonUtilS(EscapedDataObject, each_EscapedDataObject); output["EscapedDataObject"] = each_EscapedDataObject;
                return output;
            }
        };

        struct MatchmakingPlayer : public PlayFabBaseModel
        {
            Boxed<MatchmakingPlayerAttributes> Attributes;
            EntityKey Entity;

            MatchmakingPlayer() :
                PlayFabBaseModel(),
                Attributes(),
                Entity()
            {}

            MatchmakingPlayer(const MatchmakingPlayer& src) :
                PlayFabBaseModel(),
                Attributes(src.Attributes),
                Entity(src.Entity)
            {}

            ~MatchmakingPlayer() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilO(input["Attributes"], Attributes);
                FromJsonUtilO(input["Entity"], Entity);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_Attributes; ToJsonUtilO(Attributes, each_Attributes); output["Attributes"] = each_Attributes;
                Json::Value each_Entity; ToJsonUtilO(Entity, each_Entity); output["Entity"] = each_Entity;
                return output;
            }
        };

        struct CreateMatchmakingTicketRequest : public PlayFabRequestCommon
        {
            MatchmakingPlayer Creator;
            std::map<std::string, std::string> CustomTags;
            Int32 GiveUpAfterSeconds;
            std::list<EntityKey> MembersToMatchWith;
            std::string QueueName;

            CreateMatchmakingTicketRequest() :
                PlayFabRequestCommon(),
                Creator(),
                CustomTags(),
                GiveUpAfterSeconds(),
                MembersToMatchWith(),
                QueueName()
            {}

            CreateMatchmakingTicketRequest(const CreateMatchmakingTicketRequest& src) :
                PlayFabRequestCommon(),
                Creator(src.Creator),
                CustomTags(src.CustomTags),
                GiveUpAfterSeconds(src.GiveUpAfterSeconds),
                MembersToMatchWith(src.MembersToMatchWith),
                QueueName(src.QueueName)
            {}

            ~CreateMatchmakingTicketRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilO(input["Creator"], Creator);
                FromJsonUtilS(input["CustomTags"], CustomTags);
                FromJsonUtilP(input["GiveUpAfterSeconds"], GiveUpAfterSeconds);
                FromJsonUtilO(input["MembersToMatchWith"], MembersToMatchWith);
                FromJsonUtilS(input["QueueName"], QueueName);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_Creator; ToJsonUtilO(Creator, each_Creator); output["Creator"] = each_Creator;
                Json::Value each_CustomTags; ToJsonUtilS(CustomTags, each_CustomTags); output["CustomTags"] = each_CustomTags;
                Json::Value each_GiveUpAfterSeconds; ToJsonUtilP(GiveUpAfterSeconds, each_GiveUpAfterSeconds); output["GiveUpAfterSeconds"] = each_GiveUpAfterSeconds;
                Json::Value each_MembersToMatchWith; ToJsonUtilO(MembersToMatchWith, each_MembersToMatchWith); output["MembersToMatchWith"] = each_MembersToMatchWith;
                Json::Value each_QueueName; ToJsonUtilS(QueueName, each_QueueName); output["QueueName"] = each_QueueName;
                return output;
            }
        };

        struct CreateMatchmakingTicketResult : public PlayFabResultCommon
        {
            std::string TicketId;

            CreateMatchmakingTicketResult() :
                PlayFabResultCommon(),
                TicketId()
            {}

            CreateMatchmakingTicketResult(const CreateMatchmakingTicketResult& src) :
                PlayFabResultCommon(),
                TicketId(src.TicketId)
            {}

            ~CreateMatchmakingTicketResult() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["TicketId"], TicketId);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_TicketId; ToJsonUtilS(TicketId, each_TicketId); output["TicketId"] = each_TicketId;
                return output;
            }
        };

        struct CreateRemoteUserRequest : public PlayFabRequestCommon
        {
            std::string BuildId;
            std::map<std::string, std::string> CustomTags;
            Boxed<time_t> ExpirationTime;
            std::string Region;
            std::string Username;
            std::string VmId;

            CreateRemoteUserRequest() :
                PlayFabRequestCommon(),
                BuildId(),
                CustomTags(),
                ExpirationTime(),
                Region(),
                Username(),
                VmId()
            {}

            CreateRemoteUserRequest(const CreateRemoteUserRequest& src) :
                PlayFabRequestCommon(),
                BuildId(src.BuildId),
                CustomTags(src.CustomTags),
                ExpirationTime(src.ExpirationTime),
                Region(src.Region),
                Username(src.Username),
                VmId(src.VmId)
            {}

            ~CreateRemoteUserRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["BuildId"], BuildId);
                FromJsonUtilS(input["CustomTags"], CustomTags);
                FromJsonUtilT(input["ExpirationTime"], ExpirationTime);
                FromJsonUtilS(input["Region"], Region);
                FromJsonUtilS(input["Username"], Username);
                FromJsonUtilS(input["VmId"], VmId);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_BuildId; ToJsonUtilS(BuildId, each_BuildId); output["BuildId"] = each_BuildId;
                Json::Value each_CustomTags; ToJsonUtilS(CustomTags, each_CustomTags); output["CustomTags"] = each_CustomTags;
                Json::Value each_ExpirationTime; ToJsonUtilT(ExpirationTime, each_ExpirationTime); output["ExpirationTime"] = each_ExpirationTime;
                Json::Value each_Region; ToJsonUtilS(Region, each_Region); output["Region"] = each_Region;
                Json::Value each_Username; ToJsonUtilS(Username, each_Username); output["Username"] = each_Username;
                Json::Value each_VmId; ToJsonUtilS(VmId, each_VmId); output["VmId"] = each_VmId;
                return output;
            }
        };

        struct CreateRemoteUserResponse : public PlayFabResultCommon
        {
            Boxed<time_t> ExpirationTime;
            std::string Password;
            std::string Username;

            CreateRemoteUserResponse() :
                PlayFabResultCommon(),
                ExpirationTime(),
                Password(),
                Username()
            {}

            CreateRemoteUserResponse(const CreateRemoteUserResponse& src) :
                PlayFabResultCommon(),
                ExpirationTime(src.ExpirationTime),
                Password(src.Password),
                Username(src.Username)
            {}

            ~CreateRemoteUserResponse() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilT(input["ExpirationTime"], ExpirationTime);
                FromJsonUtilS(input["Password"], Password);
                FromJsonUtilS(input["Username"], Username);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_ExpirationTime; ToJsonUtilT(ExpirationTime, each_ExpirationTime); output["ExpirationTime"] = each_ExpirationTime;
                Json::Value each_Password; ToJsonUtilS(Password, each_Password); output["Password"] = each_Password;
                Json::Value each_Username; ToJsonUtilS(Username, each_Username); output["Username"] = each_Username;
                return output;
            }
        };

        struct MatchmakingPlayerWithTeamAssignment : public PlayFabBaseModel
        {
            Boxed<MatchmakingPlayerAttributes> Attributes;
            EntityKey Entity;
            std::string TeamId;

            MatchmakingPlayerWithTeamAssignment() :
                PlayFabBaseModel(),
                Attributes(),
                Entity(),
                TeamId()
            {}

            MatchmakingPlayerWithTeamAssignment(const MatchmakingPlayerWithTeamAssignment& src) :
                PlayFabBaseModel(),
                Attributes(src.Attributes),
                Entity(src.Entity),
                TeamId(src.TeamId)
            {}

            ~MatchmakingPlayerWithTeamAssignment() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilO(input["Attributes"], Attributes);
                FromJsonUtilO(input["Entity"], Entity);
                FromJsonUtilS(input["TeamId"], TeamId);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_Attributes; ToJsonUtilO(Attributes, each_Attributes); output["Attributes"] = each_Attributes;
                Json::Value each_Entity; ToJsonUtilO(Entity, each_Entity); output["Entity"] = each_Entity;
                Json::Value each_TeamId; ToJsonUtilS(TeamId, each_TeamId); output["TeamId"] = each_TeamId;
                return output;
            }
        };

        struct ServerDetails : public PlayFabBaseModel
        {
            std::string Fqdn;
            std::string IPV4Address;
            std::list<Port> Ports;
            std::string Region;

            ServerDetails() :
                PlayFabBaseModel(),
                Fqdn(),
                IPV4Address(),
                Ports(),
                Region()
            {}

            ServerDetails(const ServerDetails& src) :
                PlayFabBaseModel(),
                Fqdn(src.Fqdn),
                IPV4Address(src.IPV4Address),
                Ports(src.Ports),
                Region(src.Region)
            {}

            ~ServerDetails() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["Fqdn"], Fqdn);
                FromJsonUtilS(input["IPV4Address"], IPV4Address);
                FromJsonUtilO(input["Ports"], Ports);
                FromJsonUtilS(input["Region"], Region);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_Fqdn; ToJsonUtilS(Fqdn, each_Fqdn); output["Fqdn"] = each_Fqdn;
                Json::Value each_IPV4Address; ToJsonUtilS(IPV4Address, each_IPV4Address); output["IPV4Address"] = each_IPV4Address;
                Json::Value each_Ports; ToJsonUtilO(Ports, each_Ports); output["Ports"] = each_Ports;
                Json::Value each_Region; ToJsonUtilS(Region, each_Region); output["Region"] = each_Region;
                return output;
            }
        };

        struct CreateServerBackfillTicketRequest : public PlayFabRequestCommon
        {
            std::map<std::string, std::string> CustomTags;
            Int32 GiveUpAfterSeconds;
            std::list<MatchmakingPlayerWithTeamAssignment> Members;
            std::string QueueName;
            Boxed<ServerDetails> pfServerDetails;

            CreateServerBackfillTicketRequest() :
                PlayFabRequestCommon(),
                CustomTags(),
                GiveUpAfterSeconds(),
                Members(),
                QueueName(),
                pfServerDetails()
            {}

            CreateServerBackfillTicketRequest(const CreateServerBackfillTicketRequest& src) :
                PlayFabRequestCommon(),
                CustomTags(src.CustomTags),
                GiveUpAfterSeconds(src.GiveUpAfterSeconds),
                Members(src.Members),
                QueueName(src.QueueName),
                pfServerDetails(src.pfServerDetails)
            {}

            ~CreateServerBackfillTicketRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["CustomTags"], CustomTags);
                FromJsonUtilP(input["GiveUpAfterSeconds"], GiveUpAfterSeconds);
                FromJsonUtilO(input["Members"], Members);
                FromJsonUtilS(input["QueueName"], QueueName);
                FromJsonUtilO(input["ServerDetails"], pfServerDetails);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_CustomTags; ToJsonUtilS(CustomTags, each_CustomTags); output["CustomTags"] = each_CustomTags;
                Json::Value each_GiveUpAfterSeconds; ToJsonUtilP(GiveUpAfterSeconds, each_GiveUpAfterSeconds); output["GiveUpAfterSeconds"] = each_GiveUpAfterSeconds;
                Json::Value each_Members; ToJsonUtilO(Members, each_Members); output["Members"] = each_Members;
                Json::Value each_QueueName; ToJsonUtilS(QueueName, each_QueueName); output["QueueName"] = each_QueueName;
                Json::Value each_pfServerDetails; ToJsonUtilO(pfServerDetails, each_pfServerDetails); output["ServerDetails"] = each_pfServerDetails;
                return output;
            }
        };

        struct CreateServerBackfillTicketResult : public PlayFabResultCommon
        {
            std::string TicketId;

            CreateServerBackfillTicketResult() :
                PlayFabResultCommon(),
                TicketId()
            {}

            CreateServerBackfillTicketResult(const CreateServerBackfillTicketResult& src) :
                PlayFabResultCommon(),
                TicketId(src.TicketId)
            {}

            ~CreateServerBackfillTicketResult() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["TicketId"], TicketId);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_TicketId; ToJsonUtilS(TicketId, each_TicketId); output["TicketId"] = each_TicketId;
                return output;
            }
        };

        struct CreateServerMatchmakingTicketRequest : public PlayFabRequestCommon
        {
            std::map<std::string, std::string> CustomTags;
            Int32 GiveUpAfterSeconds;
            std::list<MatchmakingPlayer> Members;
            std::string QueueName;

            CreateServerMatchmakingTicketRequest() :
                PlayFabRequestCommon(),
                CustomTags(),
                GiveUpAfterSeconds(),
                Members(),
                QueueName()
            {}

            CreateServerMatchmakingTicketRequest(const CreateServerMatchmakingTicketRequest& src) :
                PlayFabRequestCommon(),
                CustomTags(src.CustomTags),
                GiveUpAfterSeconds(src.GiveUpAfterSeconds),
                Members(src.Members),
                QueueName(src.QueueName)
            {}

            ~CreateServerMatchmakingTicketRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["CustomTags"], CustomTags);
                FromJsonUtilP(input["GiveUpAfterSeconds"], GiveUpAfterSeconds);
                FromJsonUtilO(input["Members"], Members);
                FromJsonUtilS(input["QueueName"], QueueName);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_CustomTags; ToJsonUtilS(CustomTags, each_CustomTags); output["CustomTags"] = each_CustomTags;
                Json::Value each_GiveUpAfterSeconds; ToJsonUtilP(GiveUpAfterSeconds, each_GiveUpAfterSeconds); output["GiveUpAfterSeconds"] = each_GiveUpAfterSeconds;
                Json::Value each_Members; ToJsonUtilO(Members, each_Members); output["Members"] = each_Members;
                Json::Value each_QueueName; ToJsonUtilS(QueueName, each_QueueName); output["QueueName"] = each_QueueName;
                return output;
            }
        };

        struct CreateTitleMultiplayerServersQuotaChangeRequest : public PlayFabRequestCommon
        {
            std::string ChangeDescription;
            std::list<CoreCapacityChange> Changes;
            std::string ContactEmail;
            std::map<std::string, std::string> CustomTags;
            std::string Notes;
            Boxed<time_t> StartDate;

            CreateTitleMultiplayerServersQuotaChangeRequest() :
                PlayFabRequestCommon(),
                ChangeDescription(),
                Changes(),
                ContactEmail(),
                CustomTags(),
                Notes(),
                StartDate()
            {}

            CreateTitleMultiplayerServersQuotaChangeRequest(const CreateTitleMultiplayerServersQuotaChangeRequest& src) :
                PlayFabRequestCommon(),
                ChangeDescription(src.ChangeDescription),
                Changes(src.Changes),
                ContactEmail(src.ContactEmail),
                CustomTags(src.CustomTags),
                Notes(src.Notes),
                StartDate(src.StartDate)
            {}

            ~CreateTitleMultiplayerServersQuotaChangeRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["ChangeDescription"], ChangeDescription);
                FromJsonUtilO(input["Changes"], Changes);
                FromJsonUtilS(input["ContactEmail"], ContactEmail);
                FromJsonUtilS(input["CustomTags"], CustomTags);
                FromJsonUtilS(input["Notes"], Notes);
                FromJsonUtilT(input["StartDate"], StartDate);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_ChangeDescription; ToJsonUtilS(ChangeDescription, each_ChangeDescription); output["ChangeDescription"] = each_ChangeDescription;
                Json::Value each_Changes; ToJsonUtilO(Changes, each_Changes); output["Changes"] = each_Changes;
                Json::Value each_ContactEmail; ToJsonUtilS(ContactEmail, each_ContactEmail); output["ContactEmail"] = each_ContactEmail;
                Json::Value each_CustomTags; ToJsonUtilS(CustomTags, each_CustomTags); output["CustomTags"] = each_CustomTags;
                Json::Value each_Notes; ToJsonUtilS(Notes, each_Notes); output["Notes"] = each_Notes;
                Json::Value each_StartDate; ToJsonUtilT(StartDate, each_StartDate); output["StartDate"] = each_StartDate;
                return output;
            }
        };

        struct CreateTitleMultiplayerServersQuotaChangeResponse : public PlayFabResultCommon
        {
            std::string RequestId;
            bool WasApproved;

            CreateTitleMultiplayerServersQuotaChangeResponse() :
                PlayFabResultCommon(),
                RequestId(),
                WasApproved()
            {}

            CreateTitleMultiplayerServersQuotaChangeResponse(const CreateTitleMultiplayerServersQuotaChangeResponse& src) :
                PlayFabResultCommon(),
                RequestId(src.RequestId),
                WasApproved(src.WasApproved)
            {}

            ~CreateTitleMultiplayerServersQuotaChangeResponse() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["RequestId"], RequestId);
                FromJsonUtilP(input["WasApproved"], WasApproved);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_RequestId; ToJsonUtilS(RequestId, each_RequestId); output["RequestId"] = each_RequestId;
                Json::Value each_WasApproved; ToJsonUtilP(WasApproved, each_WasApproved); output["WasApproved"] = each_WasApproved;
                return output;
            }
        };

        struct DeleteAssetRequest : public PlayFabRequestCommon
        {
            std::map<std::string, std::string> CustomTags;
            std::string FileName;

            DeleteAssetRequest() :
                PlayFabRequestCommon(),
                CustomTags(),
                FileName()
            {}

            DeleteAssetRequest(const DeleteAssetRequest& src) :
                PlayFabRequestCommon(),
                CustomTags(src.CustomTags),
                FileName(src.FileName)
            {}

            ~DeleteAssetRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["CustomTags"], CustomTags);
                FromJsonUtilS(input["FileName"], FileName);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_CustomTags; ToJsonUtilS(CustomTags, each_CustomTags); output["CustomTags"] = each_CustomTags;
                Json::Value each_FileName; ToJsonUtilS(FileName, each_FileName); output["FileName"] = each_FileName;
                return output;
            }
        };

        struct DeleteBuildAliasRequest : public PlayFabRequestCommon
        {
            std::string AliasId;
            std::map<std::string, std::string> CustomTags;

            DeleteBuildAliasRequest() :
                PlayFabRequestCommon(),
                AliasId(),
                CustomTags()
            {}

            DeleteBuildAliasRequest(const DeleteBuildAliasRequest& src) :
                PlayFabRequestCommon(),
                AliasId(src.AliasId),
                CustomTags(src.CustomTags)
            {}

            ~DeleteBuildAliasRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["AliasId"], AliasId);
                FromJsonUtilS(input["CustomTags"], CustomTags);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_AliasId; ToJsonUtilS(AliasId, each_AliasId); output["AliasId"] = each_AliasId;
                Json::Value each_CustomTags; ToJsonUtilS(CustomTags, each_CustomTags); output["CustomTags"] = each_CustomTags;
                return output;
            }
        };

        struct DeleteBuildRegionRequest : public PlayFabRequestCommon
        {
            std::string BuildId;
            std::map<std::string, std::string> CustomTags;
            std::string Region;

            DeleteBuildRegionRequest() :
                PlayFabRequestCommon(),
                BuildId(),
                CustomTags(),
                Region()
            {}

            DeleteBuildRegionRequest(const DeleteBuildRegionRequest& src) :
                PlayFabRequestCommon(),
                BuildId(src.BuildId),
                CustomTags(src.CustomTags),
                Region(src.Region)
            {}

            ~DeleteBuildRegionRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["BuildId"], BuildId);
                FromJsonUtilS(input["CustomTags"], CustomTags);
                FromJsonUtilS(input["Region"], Region);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_BuildId; ToJsonUtilS(BuildId, each_BuildId); output["BuildId"] = each_BuildId;
                Json::Value each_CustomTags; ToJsonUtilS(CustomTags, each_CustomTags); output["CustomTags"] = each_CustomTags;
                Json::Value each_Region; ToJsonUtilS(Region, each_Region); output["Region"] = each_Region;
                return output;
            }
        };

        struct DeleteBuildRequest : public PlayFabRequestCommon
        {
            std::string BuildId;
            std::map<std::string, std::string> CustomTags;

            DeleteBuildRequest() :
                PlayFabRequestCommon(),
                BuildId(),
                CustomTags()
            {}

            DeleteBuildRequest(const DeleteBuildRequest& src) :
                PlayFabRequestCommon(),
                BuildId(src.BuildId),
                CustomTags(src.CustomTags)
            {}

            ~DeleteBuildRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["BuildId"], BuildId);
                FromJsonUtilS(input["CustomTags"], CustomTags);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_BuildId; ToJsonUtilS(BuildId, each_BuildId); output["BuildId"] = each_BuildId;
                Json::Value each_CustomTags; ToJsonUtilS(CustomTags, each_CustomTags); output["CustomTags"] = each_CustomTags;
                return output;
            }
        };

        struct DeleteCertificateRequest : public PlayFabRequestCommon
        {
            std::map<std::string, std::string> CustomTags;
            std::string Name;

            DeleteCertificateRequest() :
                PlayFabRequestCommon(),
                CustomTags(),
                Name()
            {}

            DeleteCertificateRequest(const DeleteCertificateRequest& src) :
                PlayFabRequestCommon(),
                CustomTags(src.CustomTags),
                Name(src.Name)
            {}

            ~DeleteCertificateRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["CustomTags"], CustomTags);
                FromJsonUtilS(input["Name"], Name);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_CustomTags; ToJsonUtilS(CustomTags, each_CustomTags); output["CustomTags"] = each_CustomTags;
                Json::Value each_Name; ToJsonUtilS(Name, each_Name); output["Name"] = each_Name;
                return output;
            }
        };

        struct DeleteContainerImageRequest : public PlayFabRequestCommon
        {
            std::map<std::string, std::string> CustomTags;
            std::string ImageName;

            DeleteContainerImageRequest() :
                PlayFabRequestCommon(),
                CustomTags(),
                ImageName()
            {}

            DeleteContainerImageRequest(const DeleteContainerImageRequest& src) :
                PlayFabRequestCommon(),
                CustomTags(src.CustomTags),
                ImageName(src.ImageName)
            {}

            ~DeleteContainerImageRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["CustomTags"], CustomTags);
                FromJsonUtilS(input["ImageName"], ImageName);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_CustomTags; ToJsonUtilS(CustomTags, each_CustomTags); output["CustomTags"] = each_CustomTags;
                Json::Value each_ImageName; ToJsonUtilS(ImageName, each_ImageName); output["ImageName"] = each_ImageName;
                return output;
            }
        };

        struct DeleteRemoteUserRequest : public PlayFabRequestCommon
        {
            std::string BuildId;
            std::map<std::string, std::string> CustomTags;
            std::string Region;
            std::string Username;
            std::string VmId;

            DeleteRemoteUserRequest() :
                PlayFabRequestCommon(),
                BuildId(),
                CustomTags(),
                Region(),
                Username(),
                VmId()
            {}

            DeleteRemoteUserRequest(const DeleteRemoteUserRequest& src) :
                PlayFabRequestCommon(),
                BuildId(src.BuildId),
                CustomTags(src.CustomTags),
                Region(src.Region),
                Username(src.Username),
                VmId(src.VmId)
            {}

            ~DeleteRemoteUserRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["BuildId"], BuildId);
                FromJsonUtilS(input["CustomTags"], CustomTags);
                FromJsonUtilS(input["Region"], Region);
                FromJsonUtilS(input["Username"], Username);
                FromJsonUtilS(input["VmId"], VmId);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_BuildId; ToJsonUtilS(BuildId, each_BuildId); output["BuildId"] = each_BuildId;
                Json::Value each_CustomTags; ToJsonUtilS(CustomTags, each_CustomTags); output["CustomTags"] = each_CustomTags;
                Json::Value each_Region; ToJsonUtilS(Region, each_Region); output["Region"] = each_Region;
                Json::Value each_Username; ToJsonUtilS(Username, each_Username); output["Username"] = each_Username;
                Json::Value each_VmId; ToJsonUtilS(VmId, each_VmId); output["VmId"] = each_VmId;
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

        struct EnableMultiplayerServersForTitleRequest : public PlayFabRequestCommon
        {
            std::map<std::string, std::string> CustomTags;

            EnableMultiplayerServersForTitleRequest() :
                PlayFabRequestCommon(),
                CustomTags()
            {}

            EnableMultiplayerServersForTitleRequest(const EnableMultiplayerServersForTitleRequest& src) :
                PlayFabRequestCommon(),
                CustomTags(src.CustomTags)
            {}

            ~EnableMultiplayerServersForTitleRequest() = default;

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

        struct EnableMultiplayerServersForTitleResponse : public PlayFabResultCommon
        {
            Boxed<TitleMultiplayerServerEnabledStatus> Status;

            EnableMultiplayerServersForTitleResponse() :
                PlayFabResultCommon(),
                Status()
            {}

            EnableMultiplayerServersForTitleResponse(const EnableMultiplayerServersForTitleResponse& src) :
                PlayFabResultCommon(),
                Status(src.Status)
            {}

            ~EnableMultiplayerServersForTitleResponse() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilE(input["Status"], Status);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_Status; ToJsonUtilE(Status, each_Status); output["Status"] = each_Status;
                return output;
            }
        };

        struct GetAssetDownloadUrlRequest : public PlayFabRequestCommon
        {
            std::map<std::string, std::string> CustomTags;
            std::string FileName;

            GetAssetDownloadUrlRequest() :
                PlayFabRequestCommon(),
                CustomTags(),
                FileName()
            {}

            GetAssetDownloadUrlRequest(const GetAssetDownloadUrlRequest& src) :
                PlayFabRequestCommon(),
                CustomTags(src.CustomTags),
                FileName(src.FileName)
            {}

            ~GetAssetDownloadUrlRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["CustomTags"], CustomTags);
                FromJsonUtilS(input["FileName"], FileName);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_CustomTags; ToJsonUtilS(CustomTags, each_CustomTags); output["CustomTags"] = each_CustomTags;
                Json::Value each_FileName; ToJsonUtilS(FileName, each_FileName); output["FileName"] = each_FileName;
                return output;
            }
        };

        struct GetAssetDownloadUrlResponse : public PlayFabResultCommon
        {
            std::string AssetDownloadUrl;
            std::string FileName;

            GetAssetDownloadUrlResponse() :
                PlayFabResultCommon(),
                AssetDownloadUrl(),
                FileName()
            {}

            GetAssetDownloadUrlResponse(const GetAssetDownloadUrlResponse& src) :
                PlayFabResultCommon(),
                AssetDownloadUrl(src.AssetDownloadUrl),
                FileName(src.FileName)
            {}

            ~GetAssetDownloadUrlResponse() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["AssetDownloadUrl"], AssetDownloadUrl);
                FromJsonUtilS(input["FileName"], FileName);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_AssetDownloadUrl; ToJsonUtilS(AssetDownloadUrl, each_AssetDownloadUrl); output["AssetDownloadUrl"] = each_AssetDownloadUrl;
                Json::Value each_FileName; ToJsonUtilS(FileName, each_FileName); output["FileName"] = each_FileName;
                return output;
            }
        };

        struct GetAssetUploadUrlRequest : public PlayFabRequestCommon
        {
            std::map<std::string, std::string> CustomTags;
            std::string FileName;

            GetAssetUploadUrlRequest() :
                PlayFabRequestCommon(),
                CustomTags(),
                FileName()
            {}

            GetAssetUploadUrlRequest(const GetAssetUploadUrlRequest& src) :
                PlayFabRequestCommon(),
                CustomTags(src.CustomTags),
                FileName(src.FileName)
            {}

            ~GetAssetUploadUrlRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["CustomTags"], CustomTags);
                FromJsonUtilS(input["FileName"], FileName);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_CustomTags; ToJsonUtilS(CustomTags, each_CustomTags); output["CustomTags"] = each_CustomTags;
                Json::Value each_FileName; ToJsonUtilS(FileName, each_FileName); output["FileName"] = each_FileName;
                return output;
            }
        };

        struct GetAssetUploadUrlResponse : public PlayFabResultCommon
        {
            std::string AssetUploadUrl;
            std::string FileName;

            GetAssetUploadUrlResponse() :
                PlayFabResultCommon(),
                AssetUploadUrl(),
                FileName()
            {}

            GetAssetUploadUrlResponse(const GetAssetUploadUrlResponse& src) :
                PlayFabResultCommon(),
                AssetUploadUrl(src.AssetUploadUrl),
                FileName(src.FileName)
            {}

            ~GetAssetUploadUrlResponse() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["AssetUploadUrl"], AssetUploadUrl);
                FromJsonUtilS(input["FileName"], FileName);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_AssetUploadUrl; ToJsonUtilS(AssetUploadUrl, each_AssetUploadUrl); output["AssetUploadUrl"] = each_AssetUploadUrl;
                Json::Value each_FileName; ToJsonUtilS(FileName, each_FileName); output["FileName"] = each_FileName;
                return output;
            }
        };

        struct GetBuildAliasRequest : public PlayFabRequestCommon
        {
            std::string AliasId;
            std::map<std::string, std::string> CustomTags;

            GetBuildAliasRequest() :
                PlayFabRequestCommon(),
                AliasId(),
                CustomTags()
            {}

            GetBuildAliasRequest(const GetBuildAliasRequest& src) :
                PlayFabRequestCommon(),
                AliasId(src.AliasId),
                CustomTags(src.CustomTags)
            {}

            ~GetBuildAliasRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["AliasId"], AliasId);
                FromJsonUtilS(input["CustomTags"], CustomTags);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_AliasId; ToJsonUtilS(AliasId, each_AliasId); output["AliasId"] = each_AliasId;
                Json::Value each_CustomTags; ToJsonUtilS(CustomTags, each_CustomTags); output["CustomTags"] = each_CustomTags;
                return output;
            }
        };

        struct GetBuildRequest : public PlayFabRequestCommon
        {
            std::string BuildId;
            std::map<std::string, std::string> CustomTags;

            GetBuildRequest() :
                PlayFabRequestCommon(),
                BuildId(),
                CustomTags()
            {}

            GetBuildRequest(const GetBuildRequest& src) :
                PlayFabRequestCommon(),
                BuildId(src.BuildId),
                CustomTags(src.CustomTags)
            {}

            ~GetBuildRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["BuildId"], BuildId);
                FromJsonUtilS(input["CustomTags"], CustomTags);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_BuildId; ToJsonUtilS(BuildId, each_BuildId); output["BuildId"] = each_BuildId;
                Json::Value each_CustomTags; ToJsonUtilS(CustomTags, each_CustomTags); output["CustomTags"] = each_CustomTags;
                return output;
            }
        };

        struct GetBuildResponse : public PlayFabResultCommon
        {
            Boxed<bool> AreAssetsReadonly;
            std::string BuildId;
            std::string BuildName;
            std::string BuildStatus;
            Boxed<ContainerFlavor> pfContainerFlavor;
            std::string ContainerRunCommand;
            Boxed<time_t> CreationTime;
            Boxed<ContainerImageReference> CustomGameContainerImage;
            std::list<AssetReference> GameAssetReferences;
            std::list<GameCertificateReference> GameCertificateReferences;
            Boxed<InstrumentationConfiguration> pfInstrumentationConfiguration;
            std::map<std::string, std::string> Metadata;
            Int32 MultiplayerServerCountPerVm;
            std::string OsPlatform;
            std::list<Port> Ports;
            std::list<BuildRegion> RegionConfigurations;
            std::string ServerType;
            std::string StartMultiplayerServerCommand;
            Boxed<bool> UseStreamingForAssetDownloads;
            Boxed<AzureVmSize> VmSize;

            GetBuildResponse() :
                PlayFabResultCommon(),
                AreAssetsReadonly(),
                BuildId(),
                BuildName(),
                BuildStatus(),
                pfContainerFlavor(),
                ContainerRunCommand(),
                CreationTime(),
                CustomGameContainerImage(),
                GameAssetReferences(),
                GameCertificateReferences(),
                pfInstrumentationConfiguration(),
                Metadata(),
                MultiplayerServerCountPerVm(),
                OsPlatform(),
                Ports(),
                RegionConfigurations(),
                ServerType(),
                StartMultiplayerServerCommand(),
                UseStreamingForAssetDownloads(),
                VmSize()
            {}

            GetBuildResponse(const GetBuildResponse& src) :
                PlayFabResultCommon(),
                AreAssetsReadonly(src.AreAssetsReadonly),
                BuildId(src.BuildId),
                BuildName(src.BuildName),
                BuildStatus(src.BuildStatus),
                pfContainerFlavor(src.pfContainerFlavor),
                ContainerRunCommand(src.ContainerRunCommand),
                CreationTime(src.CreationTime),
                CustomGameContainerImage(src.CustomGameContainerImage),
                GameAssetReferences(src.GameAssetReferences),
                GameCertificateReferences(src.GameCertificateReferences),
                pfInstrumentationConfiguration(src.pfInstrumentationConfiguration),
                Metadata(src.Metadata),
                MultiplayerServerCountPerVm(src.MultiplayerServerCountPerVm),
                OsPlatform(src.OsPlatform),
                Ports(src.Ports),
                RegionConfigurations(src.RegionConfigurations),
                ServerType(src.ServerType),
                StartMultiplayerServerCommand(src.StartMultiplayerServerCommand),
                UseStreamingForAssetDownloads(src.UseStreamingForAssetDownloads),
                VmSize(src.VmSize)
            {}

            ~GetBuildResponse() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilP(input["AreAssetsReadonly"], AreAssetsReadonly);
                FromJsonUtilS(input["BuildId"], BuildId);
                FromJsonUtilS(input["BuildName"], BuildName);
                FromJsonUtilS(input["BuildStatus"], BuildStatus);
                FromJsonUtilE(input["ContainerFlavor"], pfContainerFlavor);
                FromJsonUtilS(input["ContainerRunCommand"], ContainerRunCommand);
                FromJsonUtilT(input["CreationTime"], CreationTime);
                FromJsonUtilO(input["CustomGameContainerImage"], CustomGameContainerImage);
                FromJsonUtilO(input["GameAssetReferences"], GameAssetReferences);
                FromJsonUtilO(input["GameCertificateReferences"], GameCertificateReferences);
                FromJsonUtilO(input["InstrumentationConfiguration"], pfInstrumentationConfiguration);
                FromJsonUtilS(input["Metadata"], Metadata);
                FromJsonUtilP(input["MultiplayerServerCountPerVm"], MultiplayerServerCountPerVm);
                FromJsonUtilS(input["OsPlatform"], OsPlatform);
                FromJsonUtilO(input["Ports"], Ports);
                FromJsonUtilO(input["RegionConfigurations"], RegionConfigurations);
                FromJsonUtilS(input["ServerType"], ServerType);
                FromJsonUtilS(input["StartMultiplayerServerCommand"], StartMultiplayerServerCommand);
                FromJsonUtilP(input["UseStreamingForAssetDownloads"], UseStreamingForAssetDownloads);
                FromJsonUtilE(input["VmSize"], VmSize);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_AreAssetsReadonly; ToJsonUtilP(AreAssetsReadonly, each_AreAssetsReadonly); output["AreAssetsReadonly"] = each_AreAssetsReadonly;
                Json::Value each_BuildId; ToJsonUtilS(BuildId, each_BuildId); output["BuildId"] = each_BuildId;
                Json::Value each_BuildName; ToJsonUtilS(BuildName, each_BuildName); output["BuildName"] = each_BuildName;
                Json::Value each_BuildStatus; ToJsonUtilS(BuildStatus, each_BuildStatus); output["BuildStatus"] = each_BuildStatus;
                Json::Value each_pfContainerFlavor; ToJsonUtilE(pfContainerFlavor, each_pfContainerFlavor); output["ContainerFlavor"] = each_pfContainerFlavor;
                Json::Value each_ContainerRunCommand; ToJsonUtilS(ContainerRunCommand, each_ContainerRunCommand); output["ContainerRunCommand"] = each_ContainerRunCommand;
                Json::Value each_CreationTime; ToJsonUtilT(CreationTime, each_CreationTime); output["CreationTime"] = each_CreationTime;
                Json::Value each_CustomGameContainerImage; ToJsonUtilO(CustomGameContainerImage, each_CustomGameContainerImage); output["CustomGameContainerImage"] = each_CustomGameContainerImage;
                Json::Value each_GameAssetReferences; ToJsonUtilO(GameAssetReferences, each_GameAssetReferences); output["GameAssetReferences"] = each_GameAssetReferences;
                Json::Value each_GameCertificateReferences; ToJsonUtilO(GameCertificateReferences, each_GameCertificateReferences); output["GameCertificateReferences"] = each_GameCertificateReferences;
                Json::Value each_pfInstrumentationConfiguration; ToJsonUtilO(pfInstrumentationConfiguration, each_pfInstrumentationConfiguration); output["InstrumentationConfiguration"] = each_pfInstrumentationConfiguration;
                Json::Value each_Metadata; ToJsonUtilS(Metadata, each_Metadata); output["Metadata"] = each_Metadata;
                Json::Value each_MultiplayerServerCountPerVm; ToJsonUtilP(MultiplayerServerCountPerVm, each_MultiplayerServerCountPerVm); output["MultiplayerServerCountPerVm"] = each_MultiplayerServerCountPerVm;
                Json::Value each_OsPlatform; ToJsonUtilS(OsPlatform, each_OsPlatform); output["OsPlatform"] = each_OsPlatform;
                Json::Value each_Ports; ToJsonUtilO(Ports, each_Ports); output["Ports"] = each_Ports;
                Json::Value each_RegionConfigurations; ToJsonUtilO(RegionConfigurations, each_RegionConfigurations); output["RegionConfigurations"] = each_RegionConfigurations;
                Json::Value each_ServerType; ToJsonUtilS(ServerType, each_ServerType); output["ServerType"] = each_ServerType;
                Json::Value each_StartMultiplayerServerCommand; ToJsonUtilS(StartMultiplayerServerCommand, each_StartMultiplayerServerCommand); output["StartMultiplayerServerCommand"] = each_StartMultiplayerServerCommand;
                Json::Value each_UseStreamingForAssetDownloads; ToJsonUtilP(UseStreamingForAssetDownloads, each_UseStreamingForAssetDownloads); output["UseStreamingForAssetDownloads"] = each_UseStreamingForAssetDownloads;
                Json::Value each_VmSize; ToJsonUtilE(VmSize, each_VmSize); output["VmSize"] = each_VmSize;
                return output;
            }
        };

        struct GetContainerRegistryCredentialsRequest : public PlayFabRequestCommon
        {
            std::map<std::string, std::string> CustomTags;

            GetContainerRegistryCredentialsRequest() :
                PlayFabRequestCommon(),
                CustomTags()
            {}

            GetContainerRegistryCredentialsRequest(const GetContainerRegistryCredentialsRequest& src) :
                PlayFabRequestCommon(),
                CustomTags(src.CustomTags)
            {}

            ~GetContainerRegistryCredentialsRequest() = default;

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

        struct GetContainerRegistryCredentialsResponse : public PlayFabResultCommon
        {
            std::string DnsName;
            std::string Password;
            std::string Username;

            GetContainerRegistryCredentialsResponse() :
                PlayFabResultCommon(),
                DnsName(),
                Password(),
                Username()
            {}

            GetContainerRegistryCredentialsResponse(const GetContainerRegistryCredentialsResponse& src) :
                PlayFabResultCommon(),
                DnsName(src.DnsName),
                Password(src.Password),
                Username(src.Username)
            {}

            ~GetContainerRegistryCredentialsResponse() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["DnsName"], DnsName);
                FromJsonUtilS(input["Password"], Password);
                FromJsonUtilS(input["Username"], Username);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_DnsName; ToJsonUtilS(DnsName, each_DnsName); output["DnsName"] = each_DnsName;
                Json::Value each_Password; ToJsonUtilS(Password, each_Password); output["Password"] = each_Password;
                Json::Value each_Username; ToJsonUtilS(Username, each_Username); output["Username"] = each_Username;
                return output;
            }
        };

        struct GetMatchmakingTicketRequest : public PlayFabRequestCommon
        {
            std::map<std::string, std::string> CustomTags;
            bool EscapeObject;
            std::string QueueName;
            std::string TicketId;

            GetMatchmakingTicketRequest() :
                PlayFabRequestCommon(),
                CustomTags(),
                EscapeObject(),
                QueueName(),
                TicketId()
            {}

            GetMatchmakingTicketRequest(const GetMatchmakingTicketRequest& src) :
                PlayFabRequestCommon(),
                CustomTags(src.CustomTags),
                EscapeObject(src.EscapeObject),
                QueueName(src.QueueName),
                TicketId(src.TicketId)
            {}

            ~GetMatchmakingTicketRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["CustomTags"], CustomTags);
                FromJsonUtilP(input["EscapeObject"], EscapeObject);
                FromJsonUtilS(input["QueueName"], QueueName);
                FromJsonUtilS(input["TicketId"], TicketId);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_CustomTags; ToJsonUtilS(CustomTags, each_CustomTags); output["CustomTags"] = each_CustomTags;
                Json::Value each_EscapeObject; ToJsonUtilP(EscapeObject, each_EscapeObject); output["EscapeObject"] = each_EscapeObject;
                Json::Value each_QueueName; ToJsonUtilS(QueueName, each_QueueName); output["QueueName"] = each_QueueName;
                Json::Value each_TicketId; ToJsonUtilS(TicketId, each_TicketId); output["TicketId"] = each_TicketId;
                return output;
            }
        };

        struct GetMatchmakingTicketResult : public PlayFabResultCommon
        {
            std::string CancellationReasonString;
            time_t Created;
            EntityKey Creator;
            Int32 GiveUpAfterSeconds;
            std::string MatchId;
            std::list<MatchmakingPlayer> Members;
            std::list<EntityKey> MembersToMatchWith;
            std::string QueueName;
            std::string Status;
            std::string TicketId;

            GetMatchmakingTicketResult() :
                PlayFabResultCommon(),
                CancellationReasonString(),
                Created(),
                Creator(),
                GiveUpAfterSeconds(),
                MatchId(),
                Members(),
                MembersToMatchWith(),
                QueueName(),
                Status(),
                TicketId()
            {}

            GetMatchmakingTicketResult(const GetMatchmakingTicketResult& src) :
                PlayFabResultCommon(),
                CancellationReasonString(src.CancellationReasonString),
                Created(src.Created),
                Creator(src.Creator),
                GiveUpAfterSeconds(src.GiveUpAfterSeconds),
                MatchId(src.MatchId),
                Members(src.Members),
                MembersToMatchWith(src.MembersToMatchWith),
                QueueName(src.QueueName),
                Status(src.Status),
                TicketId(src.TicketId)
            {}

            ~GetMatchmakingTicketResult() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["CancellationReasonString"], CancellationReasonString);
                FromJsonUtilT(input["Created"], Created);
                FromJsonUtilO(input["Creator"], Creator);
                FromJsonUtilP(input["GiveUpAfterSeconds"], GiveUpAfterSeconds);
                FromJsonUtilS(input["MatchId"], MatchId);
                FromJsonUtilO(input["Members"], Members);
                FromJsonUtilO(input["MembersToMatchWith"], MembersToMatchWith);
                FromJsonUtilS(input["QueueName"], QueueName);
                FromJsonUtilS(input["Status"], Status);
                FromJsonUtilS(input["TicketId"], TicketId);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_CancellationReasonString; ToJsonUtilS(CancellationReasonString, each_CancellationReasonString); output["CancellationReasonString"] = each_CancellationReasonString;
                Json::Value each_Created; ToJsonUtilT(Created, each_Created); output["Created"] = each_Created;
                Json::Value each_Creator; ToJsonUtilO(Creator, each_Creator); output["Creator"] = each_Creator;
                Json::Value each_GiveUpAfterSeconds; ToJsonUtilP(GiveUpAfterSeconds, each_GiveUpAfterSeconds); output["GiveUpAfterSeconds"] = each_GiveUpAfterSeconds;
                Json::Value each_MatchId; ToJsonUtilS(MatchId, each_MatchId); output["MatchId"] = each_MatchId;
                Json::Value each_Members; ToJsonUtilO(Members, each_Members); output["Members"] = each_Members;
                Json::Value each_MembersToMatchWith; ToJsonUtilO(MembersToMatchWith, each_MembersToMatchWith); output["MembersToMatchWith"] = each_MembersToMatchWith;
                Json::Value each_QueueName; ToJsonUtilS(QueueName, each_QueueName); output["QueueName"] = each_QueueName;
                Json::Value each_Status; ToJsonUtilS(Status, each_Status); output["Status"] = each_Status;
                Json::Value each_TicketId; ToJsonUtilS(TicketId, each_TicketId); output["TicketId"] = each_TicketId;
                return output;
            }
        };

        struct GetMatchRequest : public PlayFabRequestCommon
        {
            std::map<std::string, std::string> CustomTags;
            bool EscapeObject;
            std::string MatchId;
            std::string QueueName;
            bool ReturnMemberAttributes;

            GetMatchRequest() :
                PlayFabRequestCommon(),
                CustomTags(),
                EscapeObject(),
                MatchId(),
                QueueName(),
                ReturnMemberAttributes()
            {}

            GetMatchRequest(const GetMatchRequest& src) :
                PlayFabRequestCommon(),
                CustomTags(src.CustomTags),
                EscapeObject(src.EscapeObject),
                MatchId(src.MatchId),
                QueueName(src.QueueName),
                ReturnMemberAttributes(src.ReturnMemberAttributes)
            {}

            ~GetMatchRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["CustomTags"], CustomTags);
                FromJsonUtilP(input["EscapeObject"], EscapeObject);
                FromJsonUtilS(input["MatchId"], MatchId);
                FromJsonUtilS(input["QueueName"], QueueName);
                FromJsonUtilP(input["ReturnMemberAttributes"], ReturnMemberAttributes);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_CustomTags; ToJsonUtilS(CustomTags, each_CustomTags); output["CustomTags"] = each_CustomTags;
                Json::Value each_EscapeObject; ToJsonUtilP(EscapeObject, each_EscapeObject); output["EscapeObject"] = each_EscapeObject;
                Json::Value each_MatchId; ToJsonUtilS(MatchId, each_MatchId); output["MatchId"] = each_MatchId;
                Json::Value each_QueueName; ToJsonUtilS(QueueName, each_QueueName); output["QueueName"] = each_QueueName;
                Json::Value each_ReturnMemberAttributes; ToJsonUtilP(ReturnMemberAttributes, each_ReturnMemberAttributes); output["ReturnMemberAttributes"] = each_ReturnMemberAttributes;
                return output;
            }
        };

        struct GetMatchResult : public PlayFabResultCommon
        {
            std::string MatchId;
            std::list<MatchmakingPlayerWithTeamAssignment> Members;
            std::list<std::string> RegionPreferences;
            Boxed<ServerDetails> pfServerDetails;

            GetMatchResult() :
                PlayFabResultCommon(),
                MatchId(),
                Members(),
                RegionPreferences(),
                pfServerDetails()
            {}

            GetMatchResult(const GetMatchResult& src) :
                PlayFabResultCommon(),
                MatchId(src.MatchId),
                Members(src.Members),
                RegionPreferences(src.RegionPreferences),
                pfServerDetails(src.pfServerDetails)
            {}

            ~GetMatchResult() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["MatchId"], MatchId);
                FromJsonUtilO(input["Members"], Members);
                FromJsonUtilS(input["RegionPreferences"], RegionPreferences);
                FromJsonUtilO(input["ServerDetails"], pfServerDetails);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_MatchId; ToJsonUtilS(MatchId, each_MatchId); output["MatchId"] = each_MatchId;
                Json::Value each_Members; ToJsonUtilO(Members, each_Members); output["Members"] = each_Members;
                Json::Value each_RegionPreferences; ToJsonUtilS(RegionPreferences, each_RegionPreferences); output["RegionPreferences"] = each_RegionPreferences;
                Json::Value each_pfServerDetails; ToJsonUtilO(pfServerDetails, each_pfServerDetails); output["ServerDetails"] = each_pfServerDetails;
                return output;
            }
        };

        struct GetMultiplayerServerDetailsRequest : public PlayFabRequestCommon
        {
            std::string BuildId;
            std::map<std::string, std::string> CustomTags;
            std::string Region;
            std::string SessionId;

            GetMultiplayerServerDetailsRequest() :
                PlayFabRequestCommon(),
                BuildId(),
                CustomTags(),
                Region(),
                SessionId()
            {}

            GetMultiplayerServerDetailsRequest(const GetMultiplayerServerDetailsRequest& src) :
                PlayFabRequestCommon(),
                BuildId(src.BuildId),
                CustomTags(src.CustomTags),
                Region(src.Region),
                SessionId(src.SessionId)
            {}

            ~GetMultiplayerServerDetailsRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["BuildId"], BuildId);
                FromJsonUtilS(input["CustomTags"], CustomTags);
                FromJsonUtilS(input["Region"], Region);
                FromJsonUtilS(input["SessionId"], SessionId);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_BuildId; ToJsonUtilS(BuildId, each_BuildId); output["BuildId"] = each_BuildId;
                Json::Value each_CustomTags; ToJsonUtilS(CustomTags, each_CustomTags); output["CustomTags"] = each_CustomTags;
                Json::Value each_Region; ToJsonUtilS(Region, each_Region); output["Region"] = each_Region;
                Json::Value each_SessionId; ToJsonUtilS(SessionId, each_SessionId); output["SessionId"] = each_SessionId;
                return output;
            }
        };

        struct GetMultiplayerServerDetailsResponse : public PlayFabResultCommon
        {
            std::string BuildId;
            std::list<ConnectedPlayer> ConnectedPlayers;
            std::string FQDN;
            std::string IPV4Address;
            Boxed<time_t> LastStateTransitionTime;
            std::list<Port> Ports;
            std::string Region;
            std::string ServerId;
            std::string SessionId;
            std::string State;
            std::string VmId;

            GetMultiplayerServerDetailsResponse() :
                PlayFabResultCommon(),
                BuildId(),
                ConnectedPlayers(),
                FQDN(),
                IPV4Address(),
                LastStateTransitionTime(),
                Ports(),
                Region(),
                ServerId(),
                SessionId(),
                State(),
                VmId()
            {}

            GetMultiplayerServerDetailsResponse(const GetMultiplayerServerDetailsResponse& src) :
                PlayFabResultCommon(),
                BuildId(src.BuildId),
                ConnectedPlayers(src.ConnectedPlayers),
                FQDN(src.FQDN),
                IPV4Address(src.IPV4Address),
                LastStateTransitionTime(src.LastStateTransitionTime),
                Ports(src.Ports),
                Region(src.Region),
                ServerId(src.ServerId),
                SessionId(src.SessionId),
                State(src.State),
                VmId(src.VmId)
            {}

            ~GetMultiplayerServerDetailsResponse() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["BuildId"], BuildId);
                FromJsonUtilO(input["ConnectedPlayers"], ConnectedPlayers);
                FromJsonUtilS(input["FQDN"], FQDN);
                FromJsonUtilS(input["IPV4Address"], IPV4Address);
                FromJsonUtilT(input["LastStateTransitionTime"], LastStateTransitionTime);
                FromJsonUtilO(input["Ports"], Ports);
                FromJsonUtilS(input["Region"], Region);
                FromJsonUtilS(input["ServerId"], ServerId);
                FromJsonUtilS(input["SessionId"], SessionId);
                FromJsonUtilS(input["State"], State);
                FromJsonUtilS(input["VmId"], VmId);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_BuildId; ToJsonUtilS(BuildId, each_BuildId); output["BuildId"] = each_BuildId;
                Json::Value each_ConnectedPlayers; ToJsonUtilO(ConnectedPlayers, each_ConnectedPlayers); output["ConnectedPlayers"] = each_ConnectedPlayers;
                Json::Value each_FQDN; ToJsonUtilS(FQDN, each_FQDN); output["FQDN"] = each_FQDN;
                Json::Value each_IPV4Address; ToJsonUtilS(IPV4Address, each_IPV4Address); output["IPV4Address"] = each_IPV4Address;
                Json::Value each_LastStateTransitionTime; ToJsonUtilT(LastStateTransitionTime, each_LastStateTransitionTime); output["LastStateTransitionTime"] = each_LastStateTransitionTime;
                Json::Value each_Ports; ToJsonUtilO(Ports, each_Ports); output["Ports"] = each_Ports;
                Json::Value each_Region; ToJsonUtilS(Region, each_Region); output["Region"] = each_Region;
                Json::Value each_ServerId; ToJsonUtilS(ServerId, each_ServerId); output["ServerId"] = each_ServerId;
                Json::Value each_SessionId; ToJsonUtilS(SessionId, each_SessionId); output["SessionId"] = each_SessionId;
                Json::Value each_State; ToJsonUtilS(State, each_State); output["State"] = each_State;
                Json::Value each_VmId; ToJsonUtilS(VmId, each_VmId); output["VmId"] = each_VmId;
                return output;
            }
        };

        struct GetMultiplayerServerLogsRequest : public PlayFabRequestCommon
        {
            std::map<std::string, std::string> CustomTags;
            std::string ServerId;

            GetMultiplayerServerLogsRequest() :
                PlayFabRequestCommon(),
                CustomTags(),
                ServerId()
            {}

            GetMultiplayerServerLogsRequest(const GetMultiplayerServerLogsRequest& src) :
                PlayFabRequestCommon(),
                CustomTags(src.CustomTags),
                ServerId(src.ServerId)
            {}

            ~GetMultiplayerServerLogsRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["CustomTags"], CustomTags);
                FromJsonUtilS(input["ServerId"], ServerId);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_CustomTags; ToJsonUtilS(CustomTags, each_CustomTags); output["CustomTags"] = each_CustomTags;
                Json::Value each_ServerId; ToJsonUtilS(ServerId, each_ServerId); output["ServerId"] = each_ServerId;
                return output;
            }
        };

        struct GetMultiplayerServerLogsResponse : public PlayFabResultCommon
        {
            std::string LogDownloadUrl;

            GetMultiplayerServerLogsResponse() :
                PlayFabResultCommon(),
                LogDownloadUrl()
            {}

            GetMultiplayerServerLogsResponse(const GetMultiplayerServerLogsResponse& src) :
                PlayFabResultCommon(),
                LogDownloadUrl(src.LogDownloadUrl)
            {}

            ~GetMultiplayerServerLogsResponse() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["LogDownloadUrl"], LogDownloadUrl);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_LogDownloadUrl; ToJsonUtilS(LogDownloadUrl, each_LogDownloadUrl); output["LogDownloadUrl"] = each_LogDownloadUrl;
                return output;
            }
        };

        struct GetMultiplayerSessionLogsBySessionIdRequest : public PlayFabRequestCommon
        {
            std::map<std::string, std::string> CustomTags;
            std::string SessionId;

            GetMultiplayerSessionLogsBySessionIdRequest() :
                PlayFabRequestCommon(),
                CustomTags(),
                SessionId()
            {}

            GetMultiplayerSessionLogsBySessionIdRequest(const GetMultiplayerSessionLogsBySessionIdRequest& src) :
                PlayFabRequestCommon(),
                CustomTags(src.CustomTags),
                SessionId(src.SessionId)
            {}

            ~GetMultiplayerSessionLogsBySessionIdRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["CustomTags"], CustomTags);
                FromJsonUtilS(input["SessionId"], SessionId);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_CustomTags; ToJsonUtilS(CustomTags, each_CustomTags); output["CustomTags"] = each_CustomTags;
                Json::Value each_SessionId; ToJsonUtilS(SessionId, each_SessionId); output["SessionId"] = each_SessionId;
                return output;
            }
        };

        struct GetQueueStatisticsRequest : public PlayFabRequestCommon
        {
            std::map<std::string, std::string> CustomTags;
            std::string QueueName;

            GetQueueStatisticsRequest() :
                PlayFabRequestCommon(),
                CustomTags(),
                QueueName()
            {}

            GetQueueStatisticsRequest(const GetQueueStatisticsRequest& src) :
                PlayFabRequestCommon(),
                CustomTags(src.CustomTags),
                QueueName(src.QueueName)
            {}

            ~GetQueueStatisticsRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["CustomTags"], CustomTags);
                FromJsonUtilS(input["QueueName"], QueueName);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_CustomTags; ToJsonUtilS(CustomTags, each_CustomTags); output["CustomTags"] = each_CustomTags;
                Json::Value each_QueueName; ToJsonUtilS(QueueName, each_QueueName); output["QueueName"] = each_QueueName;
                return output;
            }
        };

        struct Statistics : public PlayFabBaseModel
        {
            double Average;
            double Percentile50;
            double Percentile90;
            double Percentile99;

            Statistics() :
                PlayFabBaseModel(),
                Average(),
                Percentile50(),
                Percentile90(),
                Percentile99()
            {}

            Statistics(const Statistics& src) :
                PlayFabBaseModel(),
                Average(src.Average),
                Percentile50(src.Percentile50),
                Percentile90(src.Percentile90),
                Percentile99(src.Percentile99)
            {}

            ~Statistics() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilP(input["Average"], Average);
                FromJsonUtilP(input["Percentile50"], Percentile50);
                FromJsonUtilP(input["Percentile90"], Percentile90);
                FromJsonUtilP(input["Percentile99"], Percentile99);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_Average; ToJsonUtilP(Average, each_Average); output["Average"] = each_Average;
                Json::Value each_Percentile50; ToJsonUtilP(Percentile50, each_Percentile50); output["Percentile50"] = each_Percentile50;
                Json::Value each_Percentile90; ToJsonUtilP(Percentile90, each_Percentile90); output["Percentile90"] = each_Percentile90;
                Json::Value each_Percentile99; ToJsonUtilP(Percentile99, each_Percentile99); output["Percentile99"] = each_Percentile99;
                return output;
            }
        };

        struct GetQueueStatisticsResult : public PlayFabResultCommon
        {
            Boxed<Uint32> NumberOfPlayersMatching;
            Boxed<Statistics> TimeToMatchStatisticsInSeconds;

            GetQueueStatisticsResult() :
                PlayFabResultCommon(),
                NumberOfPlayersMatching(),
                TimeToMatchStatisticsInSeconds()
            {}

            GetQueueStatisticsResult(const GetQueueStatisticsResult& src) :
                PlayFabResultCommon(),
                NumberOfPlayersMatching(src.NumberOfPlayersMatching),
                TimeToMatchStatisticsInSeconds(src.TimeToMatchStatisticsInSeconds)
            {}

            ~GetQueueStatisticsResult() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilP(input["NumberOfPlayersMatching"], NumberOfPlayersMatching);
                FromJsonUtilO(input["TimeToMatchStatisticsInSeconds"], TimeToMatchStatisticsInSeconds);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_NumberOfPlayersMatching; ToJsonUtilP(NumberOfPlayersMatching, each_NumberOfPlayersMatching); output["NumberOfPlayersMatching"] = each_NumberOfPlayersMatching;
                Json::Value each_TimeToMatchStatisticsInSeconds; ToJsonUtilO(TimeToMatchStatisticsInSeconds, each_TimeToMatchStatisticsInSeconds); output["TimeToMatchStatisticsInSeconds"] = each_TimeToMatchStatisticsInSeconds;
                return output;
            }
        };

        struct GetRemoteLoginEndpointRequest : public PlayFabRequestCommon
        {
            std::string BuildId;
            std::map<std::string, std::string> CustomTags;
            std::string Region;
            std::string VmId;

            GetRemoteLoginEndpointRequest() :
                PlayFabRequestCommon(),
                BuildId(),
                CustomTags(),
                Region(),
                VmId()
            {}

            GetRemoteLoginEndpointRequest(const GetRemoteLoginEndpointRequest& src) :
                PlayFabRequestCommon(),
                BuildId(src.BuildId),
                CustomTags(src.CustomTags),
                Region(src.Region),
                VmId(src.VmId)
            {}

            ~GetRemoteLoginEndpointRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["BuildId"], BuildId);
                FromJsonUtilS(input["CustomTags"], CustomTags);
                FromJsonUtilS(input["Region"], Region);
                FromJsonUtilS(input["VmId"], VmId);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_BuildId; ToJsonUtilS(BuildId, each_BuildId); output["BuildId"] = each_BuildId;
                Json::Value each_CustomTags; ToJsonUtilS(CustomTags, each_CustomTags); output["CustomTags"] = each_CustomTags;
                Json::Value each_Region; ToJsonUtilS(Region, each_Region); output["Region"] = each_Region;
                Json::Value each_VmId; ToJsonUtilS(VmId, each_VmId); output["VmId"] = each_VmId;
                return output;
            }
        };

        struct GetRemoteLoginEndpointResponse : public PlayFabResultCommon
        {
            std::string IPV4Address;
            Int32 Port;

            GetRemoteLoginEndpointResponse() :
                PlayFabResultCommon(),
                IPV4Address(),
                Port()
            {}

            GetRemoteLoginEndpointResponse(const GetRemoteLoginEndpointResponse& src) :
                PlayFabResultCommon(),
                IPV4Address(src.IPV4Address),
                Port(src.Port)
            {}

            ~GetRemoteLoginEndpointResponse() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["IPV4Address"], IPV4Address);
                FromJsonUtilP(input["Port"], Port);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_IPV4Address; ToJsonUtilS(IPV4Address, each_IPV4Address); output["IPV4Address"] = each_IPV4Address;
                Json::Value each_Port; ToJsonUtilP(Port, each_Port); output["Port"] = each_Port;
                return output;
            }
        };

        struct GetServerBackfillTicketRequest : public PlayFabRequestCommon
        {
            std::map<std::string, std::string> CustomTags;
            bool EscapeObject;
            std::string QueueName;
            std::string TicketId;

            GetServerBackfillTicketRequest() :
                PlayFabRequestCommon(),
                CustomTags(),
                EscapeObject(),
                QueueName(),
                TicketId()
            {}

            GetServerBackfillTicketRequest(const GetServerBackfillTicketRequest& src) :
                PlayFabRequestCommon(),
                CustomTags(src.CustomTags),
                EscapeObject(src.EscapeObject),
                QueueName(src.QueueName),
                TicketId(src.TicketId)
            {}

            ~GetServerBackfillTicketRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["CustomTags"], CustomTags);
                FromJsonUtilP(input["EscapeObject"], EscapeObject);
                FromJsonUtilS(input["QueueName"], QueueName);
                FromJsonUtilS(input["TicketId"], TicketId);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_CustomTags; ToJsonUtilS(CustomTags, each_CustomTags); output["CustomTags"] = each_CustomTags;
                Json::Value each_EscapeObject; ToJsonUtilP(EscapeObject, each_EscapeObject); output["EscapeObject"] = each_EscapeObject;
                Json::Value each_QueueName; ToJsonUtilS(QueueName, each_QueueName); output["QueueName"] = each_QueueName;
                Json::Value each_TicketId; ToJsonUtilS(TicketId, each_TicketId); output["TicketId"] = each_TicketId;
                return output;
            }
        };

        struct GetServerBackfillTicketResult : public PlayFabResultCommon
        {
            std::string CancellationReasonString;
            time_t Created;
            Int32 GiveUpAfterSeconds;
            std::string MatchId;
            std::list<MatchmakingPlayerWithTeamAssignment> Members;
            std::string QueueName;
            ServerDetails pfServerDetails;
            std::string Status;
            std::string TicketId;

            GetServerBackfillTicketResult() :
                PlayFabResultCommon(),
                CancellationReasonString(),
                Created(),
                GiveUpAfterSeconds(),
                MatchId(),
                Members(),
                QueueName(),
                pfServerDetails(),
                Status(),
                TicketId()
            {}

            GetServerBackfillTicketResult(const GetServerBackfillTicketResult& src) :
                PlayFabResultCommon(),
                CancellationReasonString(src.CancellationReasonString),
                Created(src.Created),
                GiveUpAfterSeconds(src.GiveUpAfterSeconds),
                MatchId(src.MatchId),
                Members(src.Members),
                QueueName(src.QueueName),
                pfServerDetails(src.pfServerDetails),
                Status(src.Status),
                TicketId(src.TicketId)
            {}

            ~GetServerBackfillTicketResult() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["CancellationReasonString"], CancellationReasonString);
                FromJsonUtilT(input["Created"], Created);
                FromJsonUtilP(input["GiveUpAfterSeconds"], GiveUpAfterSeconds);
                FromJsonUtilS(input["MatchId"], MatchId);
                FromJsonUtilO(input["Members"], Members);
                FromJsonUtilS(input["QueueName"], QueueName);
                FromJsonUtilO(input["ServerDetails"], pfServerDetails);
                FromJsonUtilS(input["Status"], Status);
                FromJsonUtilS(input["TicketId"], TicketId);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_CancellationReasonString; ToJsonUtilS(CancellationReasonString, each_CancellationReasonString); output["CancellationReasonString"] = each_CancellationReasonString;
                Json::Value each_Created; ToJsonUtilT(Created, each_Created); output["Created"] = each_Created;
                Json::Value each_GiveUpAfterSeconds; ToJsonUtilP(GiveUpAfterSeconds, each_GiveUpAfterSeconds); output["GiveUpAfterSeconds"] = each_GiveUpAfterSeconds;
                Json::Value each_MatchId; ToJsonUtilS(MatchId, each_MatchId); output["MatchId"] = each_MatchId;
                Json::Value each_Members; ToJsonUtilO(Members, each_Members); output["Members"] = each_Members;
                Json::Value each_QueueName; ToJsonUtilS(QueueName, each_QueueName); output["QueueName"] = each_QueueName;
                Json::Value each_pfServerDetails; ToJsonUtilO(pfServerDetails, each_pfServerDetails); output["ServerDetails"] = each_pfServerDetails;
                Json::Value each_Status; ToJsonUtilS(Status, each_Status); output["Status"] = each_Status;
                Json::Value each_TicketId; ToJsonUtilS(TicketId, each_TicketId); output["TicketId"] = each_TicketId;
                return output;
            }
        };

        struct GetTitleEnabledForMultiplayerServersStatusRequest : public PlayFabRequestCommon
        {
            std::map<std::string, std::string> CustomTags;

            GetTitleEnabledForMultiplayerServersStatusRequest() :
                PlayFabRequestCommon(),
                CustomTags()
            {}

            GetTitleEnabledForMultiplayerServersStatusRequest(const GetTitleEnabledForMultiplayerServersStatusRequest& src) :
                PlayFabRequestCommon(),
                CustomTags(src.CustomTags)
            {}

            ~GetTitleEnabledForMultiplayerServersStatusRequest() = default;

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

        struct GetTitleEnabledForMultiplayerServersStatusResponse : public PlayFabResultCommon
        {
            Boxed<TitleMultiplayerServerEnabledStatus> Status;

            GetTitleEnabledForMultiplayerServersStatusResponse() :
                PlayFabResultCommon(),
                Status()
            {}

            GetTitleEnabledForMultiplayerServersStatusResponse(const GetTitleEnabledForMultiplayerServersStatusResponse& src) :
                PlayFabResultCommon(),
                Status(src.Status)
            {}

            ~GetTitleEnabledForMultiplayerServersStatusResponse() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilE(input["Status"], Status);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_Status; ToJsonUtilE(Status, each_Status); output["Status"] = each_Status;
                return output;
            }
        };

        struct GetTitleMultiplayerServersQuotaChangeRequest : public PlayFabRequestCommon
        {
            std::map<std::string, std::string> CustomTags;
            std::string RequestId;

            GetTitleMultiplayerServersQuotaChangeRequest() :
                PlayFabRequestCommon(),
                CustomTags(),
                RequestId()
            {}

            GetTitleMultiplayerServersQuotaChangeRequest(const GetTitleMultiplayerServersQuotaChangeRequest& src) :
                PlayFabRequestCommon(),
                CustomTags(src.CustomTags),
                RequestId(src.RequestId)
            {}

            ~GetTitleMultiplayerServersQuotaChangeRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["CustomTags"], CustomTags);
                FromJsonUtilS(input["RequestId"], RequestId);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_CustomTags; ToJsonUtilS(CustomTags, each_CustomTags); output["CustomTags"] = each_CustomTags;
                Json::Value each_RequestId; ToJsonUtilS(RequestId, each_RequestId); output["RequestId"] = each_RequestId;
                return output;
            }
        };

        struct QuotaChange : public PlayFabBaseModel
        {
            std::string ChangeDescription;
            std::list<CoreCapacityChange> Changes;
            bool IsPendingReview;
            std::string Notes;
            std::string RequestId;
            std::string ReviewComments;
            bool WasApproved;

            QuotaChange() :
                PlayFabBaseModel(),
                ChangeDescription(),
                Changes(),
                IsPendingReview(),
                Notes(),
                RequestId(),
                ReviewComments(),
                WasApproved()
            {}

            QuotaChange(const QuotaChange& src) :
                PlayFabBaseModel(),
                ChangeDescription(src.ChangeDescription),
                Changes(src.Changes),
                IsPendingReview(src.IsPendingReview),
                Notes(src.Notes),
                RequestId(src.RequestId),
                ReviewComments(src.ReviewComments),
                WasApproved(src.WasApproved)
            {}

            ~QuotaChange() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["ChangeDescription"], ChangeDescription);
                FromJsonUtilO(input["Changes"], Changes);
                FromJsonUtilP(input["IsPendingReview"], IsPendingReview);
                FromJsonUtilS(input["Notes"], Notes);
                FromJsonUtilS(input["RequestId"], RequestId);
                FromJsonUtilS(input["ReviewComments"], ReviewComments);
                FromJsonUtilP(input["WasApproved"], WasApproved);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_ChangeDescription; ToJsonUtilS(ChangeDescription, each_ChangeDescription); output["ChangeDescription"] = each_ChangeDescription;
                Json::Value each_Changes; ToJsonUtilO(Changes, each_Changes); output["Changes"] = each_Changes;
                Json::Value each_IsPendingReview; ToJsonUtilP(IsPendingReview, each_IsPendingReview); output["IsPendingReview"] = each_IsPendingReview;
                Json::Value each_Notes; ToJsonUtilS(Notes, each_Notes); output["Notes"] = each_Notes;
                Json::Value each_RequestId; ToJsonUtilS(RequestId, each_RequestId); output["RequestId"] = each_RequestId;
                Json::Value each_ReviewComments; ToJsonUtilS(ReviewComments, each_ReviewComments); output["ReviewComments"] = each_ReviewComments;
                Json::Value each_WasApproved; ToJsonUtilP(WasApproved, each_WasApproved); output["WasApproved"] = each_WasApproved;
                return output;
            }
        };

        struct GetTitleMultiplayerServersQuotaChangeResponse : public PlayFabResultCommon
        {
            Boxed<QuotaChange> Change;

            GetTitleMultiplayerServersQuotaChangeResponse() :
                PlayFabResultCommon(),
                Change()
            {}

            GetTitleMultiplayerServersQuotaChangeResponse(const GetTitleMultiplayerServersQuotaChangeResponse& src) :
                PlayFabResultCommon(),
                Change(src.Change)
            {}

            ~GetTitleMultiplayerServersQuotaChangeResponse() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilO(input["Change"], Change);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_Change; ToJsonUtilO(Change, each_Change); output["Change"] = each_Change;
                return output;
            }
        };

        struct GetTitleMultiplayerServersQuotasRequest : public PlayFabRequestCommon
        {
            std::map<std::string, std::string> CustomTags;

            GetTitleMultiplayerServersQuotasRequest() :
                PlayFabRequestCommon(),
                CustomTags()
            {}

            GetTitleMultiplayerServersQuotasRequest(const GetTitleMultiplayerServersQuotasRequest& src) :
                PlayFabRequestCommon(),
                CustomTags(src.CustomTags)
            {}

            ~GetTitleMultiplayerServersQuotasRequest() = default;

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

        struct TitleMultiplayerServersQuotas : public PlayFabBaseModel
        {
            std::list<CoreCapacity> CoreCapacities;

            TitleMultiplayerServersQuotas() :
                PlayFabBaseModel(),
                CoreCapacities()
            {}

            TitleMultiplayerServersQuotas(const TitleMultiplayerServersQuotas& src) :
                PlayFabBaseModel(),
                CoreCapacities(src.CoreCapacities)
            {}

            ~TitleMultiplayerServersQuotas() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilO(input["CoreCapacities"], CoreCapacities);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_CoreCapacities; ToJsonUtilO(CoreCapacities, each_CoreCapacities); output["CoreCapacities"] = each_CoreCapacities;
                return output;
            }
        };

        struct GetTitleMultiplayerServersQuotasResponse : public PlayFabResultCommon
        {
            Boxed<TitleMultiplayerServersQuotas> Quotas;

            GetTitleMultiplayerServersQuotasResponse() :
                PlayFabResultCommon(),
                Quotas()
            {}

            GetTitleMultiplayerServersQuotasResponse(const GetTitleMultiplayerServersQuotasResponse& src) :
                PlayFabResultCommon(),
                Quotas(src.Quotas)
            {}

            ~GetTitleMultiplayerServersQuotasResponse() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilO(input["Quotas"], Quotas);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_Quotas; ToJsonUtilO(Quotas, each_Quotas); output["Quotas"] = each_Quotas;
                return output;
            }
        };

        struct JoinMatchmakingTicketRequest : public PlayFabRequestCommon
        {
            std::map<std::string, std::string> CustomTags;
            MatchmakingPlayer Member;
            std::string QueueName;
            std::string TicketId;

            JoinMatchmakingTicketRequest() :
                PlayFabRequestCommon(),
                CustomTags(),
                Member(),
                QueueName(),
                TicketId()
            {}

            JoinMatchmakingTicketRequest(const JoinMatchmakingTicketRequest& src) :
                PlayFabRequestCommon(),
                CustomTags(src.CustomTags),
                Member(src.Member),
                QueueName(src.QueueName),
                TicketId(src.TicketId)
            {}

            ~JoinMatchmakingTicketRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["CustomTags"], CustomTags);
                FromJsonUtilO(input["Member"], Member);
                FromJsonUtilS(input["QueueName"], QueueName);
                FromJsonUtilS(input["TicketId"], TicketId);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_CustomTags; ToJsonUtilS(CustomTags, each_CustomTags); output["CustomTags"] = each_CustomTags;
                Json::Value each_Member; ToJsonUtilO(Member, each_Member); output["Member"] = each_Member;
                Json::Value each_QueueName; ToJsonUtilS(QueueName, each_QueueName); output["QueueName"] = each_QueueName;
                Json::Value each_TicketId; ToJsonUtilS(TicketId, each_TicketId); output["TicketId"] = each_TicketId;
                return output;
            }
        };

        struct JoinMatchmakingTicketResult : public PlayFabResultCommon
        {

            JoinMatchmakingTicketResult() :
                PlayFabResultCommon()
            {}

            JoinMatchmakingTicketResult(const JoinMatchmakingTicketResult&) :
                PlayFabResultCommon()
            {}

            ~JoinMatchmakingTicketResult() = default;

            void FromJson(const Json::Value&) override
            {
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                return output;
            }
        };

        struct ListAssetSummariesRequest : public PlayFabRequestCommon
        {
            std::map<std::string, std::string> CustomTags;
            Boxed<Int32> PageSize;
            std::string SkipToken;

            ListAssetSummariesRequest() :
                PlayFabRequestCommon(),
                CustomTags(),
                PageSize(),
                SkipToken()
            {}

            ListAssetSummariesRequest(const ListAssetSummariesRequest& src) :
                PlayFabRequestCommon(),
                CustomTags(src.CustomTags),
                PageSize(src.PageSize),
                SkipToken(src.SkipToken)
            {}

            ~ListAssetSummariesRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["CustomTags"], CustomTags);
                FromJsonUtilP(input["PageSize"], PageSize);
                FromJsonUtilS(input["SkipToken"], SkipToken);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_CustomTags; ToJsonUtilS(CustomTags, each_CustomTags); output["CustomTags"] = each_CustomTags;
                Json::Value each_PageSize; ToJsonUtilP(PageSize, each_PageSize); output["PageSize"] = each_PageSize;
                Json::Value each_SkipToken; ToJsonUtilS(SkipToken, each_SkipToken); output["SkipToken"] = each_SkipToken;
                return output;
            }
        };

        struct ListAssetSummariesResponse : public PlayFabResultCommon
        {
            std::list<AssetSummary> AssetSummaries;
            Int32 PageSize;
            std::string SkipToken;

            ListAssetSummariesResponse() :
                PlayFabResultCommon(),
                AssetSummaries(),
                PageSize(),
                SkipToken()
            {}

            ListAssetSummariesResponse(const ListAssetSummariesResponse& src) :
                PlayFabResultCommon(),
                AssetSummaries(src.AssetSummaries),
                PageSize(src.PageSize),
                SkipToken(src.SkipToken)
            {}

            ~ListAssetSummariesResponse() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilO(input["AssetSummaries"], AssetSummaries);
                FromJsonUtilP(input["PageSize"], PageSize);
                FromJsonUtilS(input["SkipToken"], SkipToken);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_AssetSummaries; ToJsonUtilO(AssetSummaries, each_AssetSummaries); output["AssetSummaries"] = each_AssetSummaries;
                Json::Value each_PageSize; ToJsonUtilP(PageSize, each_PageSize); output["PageSize"] = each_PageSize;
                Json::Value each_SkipToken; ToJsonUtilS(SkipToken, each_SkipToken); output["SkipToken"] = each_SkipToken;
                return output;
            }
        };

        struct ListBuildAliasesRequest : public PlayFabRequestCommon
        {
            std::map<std::string, std::string> CustomTags;
            Boxed<Int32> PageSize;
            std::string SkipToken;

            ListBuildAliasesRequest() :
                PlayFabRequestCommon(),
                CustomTags(),
                PageSize(),
                SkipToken()
            {}

            ListBuildAliasesRequest(const ListBuildAliasesRequest& src) :
                PlayFabRequestCommon(),
                CustomTags(src.CustomTags),
                PageSize(src.PageSize),
                SkipToken(src.SkipToken)
            {}

            ~ListBuildAliasesRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["CustomTags"], CustomTags);
                FromJsonUtilP(input["PageSize"], PageSize);
                FromJsonUtilS(input["SkipToken"], SkipToken);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_CustomTags; ToJsonUtilS(CustomTags, each_CustomTags); output["CustomTags"] = each_CustomTags;
                Json::Value each_PageSize; ToJsonUtilP(PageSize, each_PageSize); output["PageSize"] = each_PageSize;
                Json::Value each_SkipToken; ToJsonUtilS(SkipToken, each_SkipToken); output["SkipToken"] = each_SkipToken;
                return output;
            }
        };

        struct ListBuildAliasesResponse : public PlayFabResultCommon
        {
            std::list<BuildAliasDetailsResponse> BuildAliases;
            Int32 PageSize;
            std::string SkipToken;

            ListBuildAliasesResponse() :
                PlayFabResultCommon(),
                BuildAliases(),
                PageSize(),
                SkipToken()
            {}

            ListBuildAliasesResponse(const ListBuildAliasesResponse& src) :
                PlayFabResultCommon(),
                BuildAliases(src.BuildAliases),
                PageSize(src.PageSize),
                SkipToken(src.SkipToken)
            {}

            ~ListBuildAliasesResponse() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilO(input["BuildAliases"], BuildAliases);
                FromJsonUtilP(input["PageSize"], PageSize);
                FromJsonUtilS(input["SkipToken"], SkipToken);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_BuildAliases; ToJsonUtilO(BuildAliases, each_BuildAliases); output["BuildAliases"] = each_BuildAliases;
                Json::Value each_PageSize; ToJsonUtilP(PageSize, each_PageSize); output["PageSize"] = each_PageSize;
                Json::Value each_SkipToken; ToJsonUtilS(SkipToken, each_SkipToken); output["SkipToken"] = each_SkipToken;
                return output;
            }
        };

        struct ListBuildSummariesRequest : public PlayFabRequestCommon
        {
            std::map<std::string, std::string> CustomTags;
            Boxed<Int32> PageSize;
            std::string SkipToken;

            ListBuildSummariesRequest() :
                PlayFabRequestCommon(),
                CustomTags(),
                PageSize(),
                SkipToken()
            {}

            ListBuildSummariesRequest(const ListBuildSummariesRequest& src) :
                PlayFabRequestCommon(),
                CustomTags(src.CustomTags),
                PageSize(src.PageSize),
                SkipToken(src.SkipToken)
            {}

            ~ListBuildSummariesRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["CustomTags"], CustomTags);
                FromJsonUtilP(input["PageSize"], PageSize);
                FromJsonUtilS(input["SkipToken"], SkipToken);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_CustomTags; ToJsonUtilS(CustomTags, each_CustomTags); output["CustomTags"] = each_CustomTags;
                Json::Value each_PageSize; ToJsonUtilP(PageSize, each_PageSize); output["PageSize"] = each_PageSize;
                Json::Value each_SkipToken; ToJsonUtilS(SkipToken, each_SkipToken); output["SkipToken"] = each_SkipToken;
                return output;
            }
        };

        struct ListBuildSummariesResponse : public PlayFabResultCommon
        {
            std::list<BuildSummary> BuildSummaries;
            Int32 PageSize;
            std::string SkipToken;

            ListBuildSummariesResponse() :
                PlayFabResultCommon(),
                BuildSummaries(),
                PageSize(),
                SkipToken()
            {}

            ListBuildSummariesResponse(const ListBuildSummariesResponse& src) :
                PlayFabResultCommon(),
                BuildSummaries(src.BuildSummaries),
                PageSize(src.PageSize),
                SkipToken(src.SkipToken)
            {}

            ~ListBuildSummariesResponse() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilO(input["BuildSummaries"], BuildSummaries);
                FromJsonUtilP(input["PageSize"], PageSize);
                FromJsonUtilS(input["SkipToken"], SkipToken);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_BuildSummaries; ToJsonUtilO(BuildSummaries, each_BuildSummaries); output["BuildSummaries"] = each_BuildSummaries;
                Json::Value each_PageSize; ToJsonUtilP(PageSize, each_PageSize); output["PageSize"] = each_PageSize;
                Json::Value each_SkipToken; ToJsonUtilS(SkipToken, each_SkipToken); output["SkipToken"] = each_SkipToken;
                return output;
            }
        };

        struct ListCertificateSummariesRequest : public PlayFabRequestCommon
        {
            std::map<std::string, std::string> CustomTags;
            Boxed<Int32> PageSize;
            std::string SkipToken;

            ListCertificateSummariesRequest() :
                PlayFabRequestCommon(),
                CustomTags(),
                PageSize(),
                SkipToken()
            {}

            ListCertificateSummariesRequest(const ListCertificateSummariesRequest& src) :
                PlayFabRequestCommon(),
                CustomTags(src.CustomTags),
                PageSize(src.PageSize),
                SkipToken(src.SkipToken)
            {}

            ~ListCertificateSummariesRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["CustomTags"], CustomTags);
                FromJsonUtilP(input["PageSize"], PageSize);
                FromJsonUtilS(input["SkipToken"], SkipToken);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_CustomTags; ToJsonUtilS(CustomTags, each_CustomTags); output["CustomTags"] = each_CustomTags;
                Json::Value each_PageSize; ToJsonUtilP(PageSize, each_PageSize); output["PageSize"] = each_PageSize;
                Json::Value each_SkipToken; ToJsonUtilS(SkipToken, each_SkipToken); output["SkipToken"] = each_SkipToken;
                return output;
            }
        };

        struct ListCertificateSummariesResponse : public PlayFabResultCommon
        {
            std::list<CertificateSummary> CertificateSummaries;
            Int32 PageSize;
            std::string SkipToken;

            ListCertificateSummariesResponse() :
                PlayFabResultCommon(),
                CertificateSummaries(),
                PageSize(),
                SkipToken()
            {}

            ListCertificateSummariesResponse(const ListCertificateSummariesResponse& src) :
                PlayFabResultCommon(),
                CertificateSummaries(src.CertificateSummaries),
                PageSize(src.PageSize),
                SkipToken(src.SkipToken)
            {}

            ~ListCertificateSummariesResponse() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilO(input["CertificateSummaries"], CertificateSummaries);
                FromJsonUtilP(input["PageSize"], PageSize);
                FromJsonUtilS(input["SkipToken"], SkipToken);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_CertificateSummaries; ToJsonUtilO(CertificateSummaries, each_CertificateSummaries); output["CertificateSummaries"] = each_CertificateSummaries;
                Json::Value each_PageSize; ToJsonUtilP(PageSize, each_PageSize); output["PageSize"] = each_PageSize;
                Json::Value each_SkipToken; ToJsonUtilS(SkipToken, each_SkipToken); output["SkipToken"] = each_SkipToken;
                return output;
            }
        };

        struct ListContainerImagesRequest : public PlayFabRequestCommon
        {
            std::map<std::string, std::string> CustomTags;
            Boxed<Int32> PageSize;
            std::string SkipToken;

            ListContainerImagesRequest() :
                PlayFabRequestCommon(),
                CustomTags(),
                PageSize(),
                SkipToken()
            {}

            ListContainerImagesRequest(const ListContainerImagesRequest& src) :
                PlayFabRequestCommon(),
                CustomTags(src.CustomTags),
                PageSize(src.PageSize),
                SkipToken(src.SkipToken)
            {}

            ~ListContainerImagesRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["CustomTags"], CustomTags);
                FromJsonUtilP(input["PageSize"], PageSize);
                FromJsonUtilS(input["SkipToken"], SkipToken);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_CustomTags; ToJsonUtilS(CustomTags, each_CustomTags); output["CustomTags"] = each_CustomTags;
                Json::Value each_PageSize; ToJsonUtilP(PageSize, each_PageSize); output["PageSize"] = each_PageSize;
                Json::Value each_SkipToken; ToJsonUtilS(SkipToken, each_SkipToken); output["SkipToken"] = each_SkipToken;
                return output;
            }
        };

        struct ListContainerImagesResponse : public PlayFabResultCommon
        {
            std::list<std::string> Images;
            Int32 PageSize;
            std::string SkipToken;

            ListContainerImagesResponse() :
                PlayFabResultCommon(),
                Images(),
                PageSize(),
                SkipToken()
            {}

            ListContainerImagesResponse(const ListContainerImagesResponse& src) :
                PlayFabResultCommon(),
                Images(src.Images),
                PageSize(src.PageSize),
                SkipToken(src.SkipToken)
            {}

            ~ListContainerImagesResponse() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["Images"], Images);
                FromJsonUtilP(input["PageSize"], PageSize);
                FromJsonUtilS(input["SkipToken"], SkipToken);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_Images; ToJsonUtilS(Images, each_Images); output["Images"] = each_Images;
                Json::Value each_PageSize; ToJsonUtilP(PageSize, each_PageSize); output["PageSize"] = each_PageSize;
                Json::Value each_SkipToken; ToJsonUtilS(SkipToken, each_SkipToken); output["SkipToken"] = each_SkipToken;
                return output;
            }
        };

        struct ListContainerImageTagsRequest : public PlayFabRequestCommon
        {
            std::map<std::string, std::string> CustomTags;
            std::string ImageName;

            ListContainerImageTagsRequest() :
                PlayFabRequestCommon(),
                CustomTags(),
                ImageName()
            {}

            ListContainerImageTagsRequest(const ListContainerImageTagsRequest& src) :
                PlayFabRequestCommon(),
                CustomTags(src.CustomTags),
                ImageName(src.ImageName)
            {}

            ~ListContainerImageTagsRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["CustomTags"], CustomTags);
                FromJsonUtilS(input["ImageName"], ImageName);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_CustomTags; ToJsonUtilS(CustomTags, each_CustomTags); output["CustomTags"] = each_CustomTags;
                Json::Value each_ImageName; ToJsonUtilS(ImageName, each_ImageName); output["ImageName"] = each_ImageName;
                return output;
            }
        };

        struct ListContainerImageTagsResponse : public PlayFabResultCommon
        {
            std::list<std::string> Tags;

            ListContainerImageTagsResponse() :
                PlayFabResultCommon(),
                Tags()
            {}

            ListContainerImageTagsResponse(const ListContainerImageTagsResponse& src) :
                PlayFabResultCommon(),
                Tags(src.Tags)
            {}

            ~ListContainerImageTagsResponse() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["Tags"], Tags);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_Tags; ToJsonUtilS(Tags, each_Tags); output["Tags"] = each_Tags;
                return output;
            }
        };

        struct ListMatchmakingTicketsForPlayerRequest : public PlayFabRequestCommon
        {
            std::map<std::string, std::string> CustomTags;
            Boxed<EntityKey> Entity;
            std::string QueueName;

            ListMatchmakingTicketsForPlayerRequest() :
                PlayFabRequestCommon(),
                CustomTags(),
                Entity(),
                QueueName()
            {}

            ListMatchmakingTicketsForPlayerRequest(const ListMatchmakingTicketsForPlayerRequest& src) :
                PlayFabRequestCommon(),
                CustomTags(src.CustomTags),
                Entity(src.Entity),
                QueueName(src.QueueName)
            {}

            ~ListMatchmakingTicketsForPlayerRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["CustomTags"], CustomTags);
                FromJsonUtilO(input["Entity"], Entity);
                FromJsonUtilS(input["QueueName"], QueueName);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_CustomTags; ToJsonUtilS(CustomTags, each_CustomTags); output["CustomTags"] = each_CustomTags;
                Json::Value each_Entity; ToJsonUtilO(Entity, each_Entity); output["Entity"] = each_Entity;
                Json::Value each_QueueName; ToJsonUtilS(QueueName, each_QueueName); output["QueueName"] = each_QueueName;
                return output;
            }
        };

        struct ListMatchmakingTicketsForPlayerResult : public PlayFabResultCommon
        {
            std::list<std::string> TicketIds;

            ListMatchmakingTicketsForPlayerResult() :
                PlayFabResultCommon(),
                TicketIds()
            {}

            ListMatchmakingTicketsForPlayerResult(const ListMatchmakingTicketsForPlayerResult& src) :
                PlayFabResultCommon(),
                TicketIds(src.TicketIds)
            {}

            ~ListMatchmakingTicketsForPlayerResult() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["TicketIds"], TicketIds);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_TicketIds; ToJsonUtilS(TicketIds, each_TicketIds); output["TicketIds"] = each_TicketIds;
                return output;
            }
        };

        struct ListMultiplayerServersRequest : public PlayFabRequestCommon
        {
            std::string BuildId;
            std::map<std::string, std::string> CustomTags;
            Boxed<Int32> PageSize;
            std::string Region;
            std::string SkipToken;

            ListMultiplayerServersRequest() :
                PlayFabRequestCommon(),
                BuildId(),
                CustomTags(),
                PageSize(),
                Region(),
                SkipToken()
            {}

            ListMultiplayerServersRequest(const ListMultiplayerServersRequest& src) :
                PlayFabRequestCommon(),
                BuildId(src.BuildId),
                CustomTags(src.CustomTags),
                PageSize(src.PageSize),
                Region(src.Region),
                SkipToken(src.SkipToken)
            {}

            ~ListMultiplayerServersRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["BuildId"], BuildId);
                FromJsonUtilS(input["CustomTags"], CustomTags);
                FromJsonUtilP(input["PageSize"], PageSize);
                FromJsonUtilS(input["Region"], Region);
                FromJsonUtilS(input["SkipToken"], SkipToken);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_BuildId; ToJsonUtilS(BuildId, each_BuildId); output["BuildId"] = each_BuildId;
                Json::Value each_CustomTags; ToJsonUtilS(CustomTags, each_CustomTags); output["CustomTags"] = each_CustomTags;
                Json::Value each_PageSize; ToJsonUtilP(PageSize, each_PageSize); output["PageSize"] = each_PageSize;
                Json::Value each_Region; ToJsonUtilS(Region, each_Region); output["Region"] = each_Region;
                Json::Value each_SkipToken; ToJsonUtilS(SkipToken, each_SkipToken); output["SkipToken"] = each_SkipToken;
                return output;
            }
        };

        struct MultiplayerServerSummary : public PlayFabBaseModel
        {
            std::list<ConnectedPlayer> ConnectedPlayers;
            Boxed<time_t> LastStateTransitionTime;
            std::string Region;
            std::string ServerId;
            std::string SessionId;
            std::string State;
            std::string VmId;

            MultiplayerServerSummary() :
                PlayFabBaseModel(),
                ConnectedPlayers(),
                LastStateTransitionTime(),
                Region(),
                ServerId(),
                SessionId(),
                State(),
                VmId()
            {}

            MultiplayerServerSummary(const MultiplayerServerSummary& src) :
                PlayFabBaseModel(),
                ConnectedPlayers(src.ConnectedPlayers),
                LastStateTransitionTime(src.LastStateTransitionTime),
                Region(src.Region),
                ServerId(src.ServerId),
                SessionId(src.SessionId),
                State(src.State),
                VmId(src.VmId)
            {}

            ~MultiplayerServerSummary() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilO(input["ConnectedPlayers"], ConnectedPlayers);
                FromJsonUtilT(input["LastStateTransitionTime"], LastStateTransitionTime);
                FromJsonUtilS(input["Region"], Region);
                FromJsonUtilS(input["ServerId"], ServerId);
                FromJsonUtilS(input["SessionId"], SessionId);
                FromJsonUtilS(input["State"], State);
                FromJsonUtilS(input["VmId"], VmId);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_ConnectedPlayers; ToJsonUtilO(ConnectedPlayers, each_ConnectedPlayers); output["ConnectedPlayers"] = each_ConnectedPlayers;
                Json::Value each_LastStateTransitionTime; ToJsonUtilT(LastStateTransitionTime, each_LastStateTransitionTime); output["LastStateTransitionTime"] = each_LastStateTransitionTime;
                Json::Value each_Region; ToJsonUtilS(Region, each_Region); output["Region"] = each_Region;
                Json::Value each_ServerId; ToJsonUtilS(ServerId, each_ServerId); output["ServerId"] = each_ServerId;
                Json::Value each_SessionId; ToJsonUtilS(SessionId, each_SessionId); output["SessionId"] = each_SessionId;
                Json::Value each_State; ToJsonUtilS(State, each_State); output["State"] = each_State;
                Json::Value each_VmId; ToJsonUtilS(VmId, each_VmId); output["VmId"] = each_VmId;
                return output;
            }
        };

        struct ListMultiplayerServersResponse : public PlayFabResultCommon
        {
            std::list<MultiplayerServerSummary> MultiplayerServerSummaries;
            Int32 PageSize;
            std::string SkipToken;

            ListMultiplayerServersResponse() :
                PlayFabResultCommon(),
                MultiplayerServerSummaries(),
                PageSize(),
                SkipToken()
            {}

            ListMultiplayerServersResponse(const ListMultiplayerServersResponse& src) :
                PlayFabResultCommon(),
                MultiplayerServerSummaries(src.MultiplayerServerSummaries),
                PageSize(src.PageSize),
                SkipToken(src.SkipToken)
            {}

            ~ListMultiplayerServersResponse() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilO(input["MultiplayerServerSummaries"], MultiplayerServerSummaries);
                FromJsonUtilP(input["PageSize"], PageSize);
                FromJsonUtilS(input["SkipToken"], SkipToken);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_MultiplayerServerSummaries; ToJsonUtilO(MultiplayerServerSummaries, each_MultiplayerServerSummaries); output["MultiplayerServerSummaries"] = each_MultiplayerServerSummaries;
                Json::Value each_PageSize; ToJsonUtilP(PageSize, each_PageSize); output["PageSize"] = each_PageSize;
                Json::Value each_SkipToken; ToJsonUtilS(SkipToken, each_SkipToken); output["SkipToken"] = each_SkipToken;
                return output;
            }
        };

        struct ListPartyQosServersRequest : public PlayFabRequestCommon
        {
            std::map<std::string, std::string> CustomTags;

            ListPartyQosServersRequest() :
                PlayFabRequestCommon(),
                CustomTags()
            {}

            ListPartyQosServersRequest(const ListPartyQosServersRequest& src) :
                PlayFabRequestCommon(),
                CustomTags(src.CustomTags)
            {}

            ~ListPartyQosServersRequest() = default;

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

        struct QosServer : public PlayFabBaseModel
        {
            std::string Region;
            std::string ServerUrl;

            QosServer() :
                PlayFabBaseModel(),
                Region(),
                ServerUrl()
            {}

            QosServer(const QosServer& src) :
                PlayFabBaseModel(),
                Region(src.Region),
                ServerUrl(src.ServerUrl)
            {}

            ~QosServer() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["Region"], Region);
                FromJsonUtilS(input["ServerUrl"], ServerUrl);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_Region; ToJsonUtilS(Region, each_Region); output["Region"] = each_Region;
                Json::Value each_ServerUrl; ToJsonUtilS(ServerUrl, each_ServerUrl); output["ServerUrl"] = each_ServerUrl;
                return output;
            }
        };

        struct ListPartyQosServersResponse : public PlayFabResultCommon
        {
            Int32 PageSize;
            std::list<QosServer> QosServers;
            std::string SkipToken;

            ListPartyQosServersResponse() :
                PlayFabResultCommon(),
                PageSize(),
                QosServers(),
                SkipToken()
            {}

            ListPartyQosServersResponse(const ListPartyQosServersResponse& src) :
                PlayFabResultCommon(),
                PageSize(src.PageSize),
                QosServers(src.QosServers),
                SkipToken(src.SkipToken)
            {}

            ~ListPartyQosServersResponse() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilP(input["PageSize"], PageSize);
                FromJsonUtilO(input["QosServers"], QosServers);
                FromJsonUtilS(input["SkipToken"], SkipToken);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_PageSize; ToJsonUtilP(PageSize, each_PageSize); output["PageSize"] = each_PageSize;
                Json::Value each_QosServers; ToJsonUtilO(QosServers, each_QosServers); output["QosServers"] = each_QosServers;
                Json::Value each_SkipToken; ToJsonUtilS(SkipToken, each_SkipToken); output["SkipToken"] = each_SkipToken;
                return output;
            }
        };

        struct ListQosServersForTitleRequest : public PlayFabRequestCommon
        {
            std::map<std::string, std::string> CustomTags;
            Boxed<bool> IncludeAllRegions;

            ListQosServersForTitleRequest() :
                PlayFabRequestCommon(),
                CustomTags(),
                IncludeAllRegions()
            {}

            ListQosServersForTitleRequest(const ListQosServersForTitleRequest& src) :
                PlayFabRequestCommon(),
                CustomTags(src.CustomTags),
                IncludeAllRegions(src.IncludeAllRegions)
            {}

            ~ListQosServersForTitleRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["CustomTags"], CustomTags);
                FromJsonUtilP(input["IncludeAllRegions"], IncludeAllRegions);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_CustomTags; ToJsonUtilS(CustomTags, each_CustomTags); output["CustomTags"] = each_CustomTags;
                Json::Value each_IncludeAllRegions; ToJsonUtilP(IncludeAllRegions, each_IncludeAllRegions); output["IncludeAllRegions"] = each_IncludeAllRegions;
                return output;
            }
        };

        struct ListQosServersForTitleResponse : public PlayFabResultCommon
        {
            Int32 PageSize;
            std::list<QosServer> QosServers;
            std::string SkipToken;

            ListQosServersForTitleResponse() :
                PlayFabResultCommon(),
                PageSize(),
                QosServers(),
                SkipToken()
            {}

            ListQosServersForTitleResponse(const ListQosServersForTitleResponse& src) :
                PlayFabResultCommon(),
                PageSize(src.PageSize),
                QosServers(src.QosServers),
                SkipToken(src.SkipToken)
            {}

            ~ListQosServersForTitleResponse() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilP(input["PageSize"], PageSize);
                FromJsonUtilO(input["QosServers"], QosServers);
                FromJsonUtilS(input["SkipToken"], SkipToken);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_PageSize; ToJsonUtilP(PageSize, each_PageSize); output["PageSize"] = each_PageSize;
                Json::Value each_QosServers; ToJsonUtilO(QosServers, each_QosServers); output["QosServers"] = each_QosServers;
                Json::Value each_SkipToken; ToJsonUtilS(SkipToken, each_SkipToken); output["SkipToken"] = each_SkipToken;
                return output;
            }
        };

        struct ListServerBackfillTicketsForPlayerRequest : public PlayFabRequestCommon
        {
            std::map<std::string, std::string> CustomTags;
            EntityKey Entity;
            std::string QueueName;

            ListServerBackfillTicketsForPlayerRequest() :
                PlayFabRequestCommon(),
                CustomTags(),
                Entity(),
                QueueName()
            {}

            ListServerBackfillTicketsForPlayerRequest(const ListServerBackfillTicketsForPlayerRequest& src) :
                PlayFabRequestCommon(),
                CustomTags(src.CustomTags),
                Entity(src.Entity),
                QueueName(src.QueueName)
            {}

            ~ListServerBackfillTicketsForPlayerRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["CustomTags"], CustomTags);
                FromJsonUtilO(input["Entity"], Entity);
                FromJsonUtilS(input["QueueName"], QueueName);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_CustomTags; ToJsonUtilS(CustomTags, each_CustomTags); output["CustomTags"] = each_CustomTags;
                Json::Value each_Entity; ToJsonUtilO(Entity, each_Entity); output["Entity"] = each_Entity;
                Json::Value each_QueueName; ToJsonUtilS(QueueName, each_QueueName); output["QueueName"] = each_QueueName;
                return output;
            }
        };

        struct ListServerBackfillTicketsForPlayerResult : public PlayFabResultCommon
        {
            std::list<std::string> TicketIds;

            ListServerBackfillTicketsForPlayerResult() :
                PlayFabResultCommon(),
                TicketIds()
            {}

            ListServerBackfillTicketsForPlayerResult(const ListServerBackfillTicketsForPlayerResult& src) :
                PlayFabResultCommon(),
                TicketIds(src.TicketIds)
            {}

            ~ListServerBackfillTicketsForPlayerResult() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["TicketIds"], TicketIds);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_TicketIds; ToJsonUtilS(TicketIds, each_TicketIds); output["TicketIds"] = each_TicketIds;
                return output;
            }
        };

        struct ListTitleMultiplayerServersQuotaChangesRequest : public PlayFabRequestCommon
        {
            std::map<std::string, std::string> CustomTags;

            ListTitleMultiplayerServersQuotaChangesRequest() :
                PlayFabRequestCommon(),
                CustomTags()
            {}

            ListTitleMultiplayerServersQuotaChangesRequest(const ListTitleMultiplayerServersQuotaChangesRequest& src) :
                PlayFabRequestCommon(),
                CustomTags(src.CustomTags)
            {}

            ~ListTitleMultiplayerServersQuotaChangesRequest() = default;

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

        struct ListTitleMultiplayerServersQuotaChangesResponse : public PlayFabResultCommon
        {
            std::list<QuotaChange> Changes;

            ListTitleMultiplayerServersQuotaChangesResponse() :
                PlayFabResultCommon(),
                Changes()
            {}

            ListTitleMultiplayerServersQuotaChangesResponse(const ListTitleMultiplayerServersQuotaChangesResponse& src) :
                PlayFabResultCommon(),
                Changes(src.Changes)
            {}

            ~ListTitleMultiplayerServersQuotaChangesResponse() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilO(input["Changes"], Changes);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_Changes; ToJsonUtilO(Changes, each_Changes); output["Changes"] = each_Changes;
                return output;
            }
        };

        struct ListVirtualMachineSummariesRequest : public PlayFabRequestCommon
        {
            std::string BuildId;
            std::map<std::string, std::string> CustomTags;
            Boxed<Int32> PageSize;
            std::string Region;
            std::string SkipToken;

            ListVirtualMachineSummariesRequest() :
                PlayFabRequestCommon(),
                BuildId(),
                CustomTags(),
                PageSize(),
                Region(),
                SkipToken()
            {}

            ListVirtualMachineSummariesRequest(const ListVirtualMachineSummariesRequest& src) :
                PlayFabRequestCommon(),
                BuildId(src.BuildId),
                CustomTags(src.CustomTags),
                PageSize(src.PageSize),
                Region(src.Region),
                SkipToken(src.SkipToken)
            {}

            ~ListVirtualMachineSummariesRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["BuildId"], BuildId);
                FromJsonUtilS(input["CustomTags"], CustomTags);
                FromJsonUtilP(input["PageSize"], PageSize);
                FromJsonUtilS(input["Region"], Region);
                FromJsonUtilS(input["SkipToken"], SkipToken);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_BuildId; ToJsonUtilS(BuildId, each_BuildId); output["BuildId"] = each_BuildId;
                Json::Value each_CustomTags; ToJsonUtilS(CustomTags, each_CustomTags); output["CustomTags"] = each_CustomTags;
                Json::Value each_PageSize; ToJsonUtilP(PageSize, each_PageSize); output["PageSize"] = each_PageSize;
                Json::Value each_Region; ToJsonUtilS(Region, each_Region); output["Region"] = each_Region;
                Json::Value each_SkipToken; ToJsonUtilS(SkipToken, each_SkipToken); output["SkipToken"] = each_SkipToken;
                return output;
            }
        };

        struct VirtualMachineSummary : public PlayFabBaseModel
        {
            std::string HealthStatus;
            std::string State;
            std::string VmId;

            VirtualMachineSummary() :
                PlayFabBaseModel(),
                HealthStatus(),
                State(),
                VmId()
            {}

            VirtualMachineSummary(const VirtualMachineSummary& src) :
                PlayFabBaseModel(),
                HealthStatus(src.HealthStatus),
                State(src.State),
                VmId(src.VmId)
            {}

            ~VirtualMachineSummary() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["HealthStatus"], HealthStatus);
                FromJsonUtilS(input["State"], State);
                FromJsonUtilS(input["VmId"], VmId);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_HealthStatus; ToJsonUtilS(HealthStatus, each_HealthStatus); output["HealthStatus"] = each_HealthStatus;
                Json::Value each_State; ToJsonUtilS(State, each_State); output["State"] = each_State;
                Json::Value each_VmId; ToJsonUtilS(VmId, each_VmId); output["VmId"] = each_VmId;
                return output;
            }
        };

        struct ListVirtualMachineSummariesResponse : public PlayFabResultCommon
        {
            Int32 PageSize;
            std::string SkipToken;
            std::list<VirtualMachineSummary> VirtualMachines;

            ListVirtualMachineSummariesResponse() :
                PlayFabResultCommon(),
                PageSize(),
                SkipToken(),
                VirtualMachines()
            {}

            ListVirtualMachineSummariesResponse(const ListVirtualMachineSummariesResponse& src) :
                PlayFabResultCommon(),
                PageSize(src.PageSize),
                SkipToken(src.SkipToken),
                VirtualMachines(src.VirtualMachines)
            {}

            ~ListVirtualMachineSummariesResponse() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilP(input["PageSize"], PageSize);
                FromJsonUtilS(input["SkipToken"], SkipToken);
                FromJsonUtilO(input["VirtualMachines"], VirtualMachines);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_PageSize; ToJsonUtilP(PageSize, each_PageSize); output["PageSize"] = each_PageSize;
                Json::Value each_SkipToken; ToJsonUtilS(SkipToken, each_SkipToken); output["SkipToken"] = each_SkipToken;
                Json::Value each_VirtualMachines; ToJsonUtilO(VirtualMachines, each_VirtualMachines); output["VirtualMachines"] = each_VirtualMachines;
                return output;
            }
        };

        struct RequestMultiplayerServerRequest : public PlayFabRequestCommon
        {
            Boxed<BuildAliasParams> pfBuildAliasParams;
            std::string BuildId;
            std::map<std::string, std::string> CustomTags;
            std::list<std::string> InitialPlayers;
            std::list<std::string> PreferredRegions;
            std::string SessionCookie;
            std::string SessionId;

            RequestMultiplayerServerRequest() :
                PlayFabRequestCommon(),
                pfBuildAliasParams(),
                BuildId(),
                CustomTags(),
                InitialPlayers(),
                PreferredRegions(),
                SessionCookie(),
                SessionId()
            {}

            RequestMultiplayerServerRequest(const RequestMultiplayerServerRequest& src) :
                PlayFabRequestCommon(),
                pfBuildAliasParams(src.pfBuildAliasParams),
                BuildId(src.BuildId),
                CustomTags(src.CustomTags),
                InitialPlayers(src.InitialPlayers),
                PreferredRegions(src.PreferredRegions),
                SessionCookie(src.SessionCookie),
                SessionId(src.SessionId)
            {}

            ~RequestMultiplayerServerRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilO(input["BuildAliasParams"], pfBuildAliasParams);
                FromJsonUtilS(input["BuildId"], BuildId);
                FromJsonUtilS(input["CustomTags"], CustomTags);
                FromJsonUtilS(input["InitialPlayers"], InitialPlayers);
                FromJsonUtilS(input["PreferredRegions"], PreferredRegions);
                FromJsonUtilS(input["SessionCookie"], SessionCookie);
                FromJsonUtilS(input["SessionId"], SessionId);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_pfBuildAliasParams; ToJsonUtilO(pfBuildAliasParams, each_pfBuildAliasParams); output["BuildAliasParams"] = each_pfBuildAliasParams;
                Json::Value each_BuildId; ToJsonUtilS(BuildId, each_BuildId); output["BuildId"] = each_BuildId;
                Json::Value each_CustomTags; ToJsonUtilS(CustomTags, each_CustomTags); output["CustomTags"] = each_CustomTags;
                Json::Value each_InitialPlayers; ToJsonUtilS(InitialPlayers, each_InitialPlayers); output["InitialPlayers"] = each_InitialPlayers;
                Json::Value each_PreferredRegions; ToJsonUtilS(PreferredRegions, each_PreferredRegions); output["PreferredRegions"] = each_PreferredRegions;
                Json::Value each_SessionCookie; ToJsonUtilS(SessionCookie, each_SessionCookie); output["SessionCookie"] = each_SessionCookie;
                Json::Value each_SessionId; ToJsonUtilS(SessionId, each_SessionId); output["SessionId"] = each_SessionId;
                return output;
            }
        };

        struct RequestMultiplayerServerResponse : public PlayFabResultCommon
        {
            std::string BuildId;
            std::list<ConnectedPlayer> ConnectedPlayers;
            std::string FQDN;
            std::string IPV4Address;
            Boxed<time_t> LastStateTransitionTime;
            std::list<Port> Ports;
            std::string Region;
            std::string ServerId;
            std::string SessionId;
            std::string State;
            std::string VmId;

            RequestMultiplayerServerResponse() :
                PlayFabResultCommon(),
                BuildId(),
                ConnectedPlayers(),
                FQDN(),
                IPV4Address(),
                LastStateTransitionTime(),
                Ports(),
                Region(),
                ServerId(),
                SessionId(),
                State(),
                VmId()
            {}

            RequestMultiplayerServerResponse(const RequestMultiplayerServerResponse& src) :
                PlayFabResultCommon(),
                BuildId(src.BuildId),
                ConnectedPlayers(src.ConnectedPlayers),
                FQDN(src.FQDN),
                IPV4Address(src.IPV4Address),
                LastStateTransitionTime(src.LastStateTransitionTime),
                Ports(src.Ports),
                Region(src.Region),
                ServerId(src.ServerId),
                SessionId(src.SessionId),
                State(src.State),
                VmId(src.VmId)
            {}

            ~RequestMultiplayerServerResponse() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["BuildId"], BuildId);
                FromJsonUtilO(input["ConnectedPlayers"], ConnectedPlayers);
                FromJsonUtilS(input["FQDN"], FQDN);
                FromJsonUtilS(input["IPV4Address"], IPV4Address);
                FromJsonUtilT(input["LastStateTransitionTime"], LastStateTransitionTime);
                FromJsonUtilO(input["Ports"], Ports);
                FromJsonUtilS(input["Region"], Region);
                FromJsonUtilS(input["ServerId"], ServerId);
                FromJsonUtilS(input["SessionId"], SessionId);
                FromJsonUtilS(input["State"], State);
                FromJsonUtilS(input["VmId"], VmId);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_BuildId; ToJsonUtilS(BuildId, each_BuildId); output["BuildId"] = each_BuildId;
                Json::Value each_ConnectedPlayers; ToJsonUtilO(ConnectedPlayers, each_ConnectedPlayers); output["ConnectedPlayers"] = each_ConnectedPlayers;
                Json::Value each_FQDN; ToJsonUtilS(FQDN, each_FQDN); output["FQDN"] = each_FQDN;
                Json::Value each_IPV4Address; ToJsonUtilS(IPV4Address, each_IPV4Address); output["IPV4Address"] = each_IPV4Address;
                Json::Value each_LastStateTransitionTime; ToJsonUtilT(LastStateTransitionTime, each_LastStateTransitionTime); output["LastStateTransitionTime"] = each_LastStateTransitionTime;
                Json::Value each_Ports; ToJsonUtilO(Ports, each_Ports); output["Ports"] = each_Ports;
                Json::Value each_Region; ToJsonUtilS(Region, each_Region); output["Region"] = each_Region;
                Json::Value each_ServerId; ToJsonUtilS(ServerId, each_ServerId); output["ServerId"] = each_ServerId;
                Json::Value each_SessionId; ToJsonUtilS(SessionId, each_SessionId); output["SessionId"] = each_SessionId;
                Json::Value each_State; ToJsonUtilS(State, each_State); output["State"] = each_State;
                Json::Value each_VmId; ToJsonUtilS(VmId, each_VmId); output["VmId"] = each_VmId;
                return output;
            }
        };

        struct RolloverContainerRegistryCredentialsRequest : public PlayFabRequestCommon
        {
            std::map<std::string, std::string> CustomTags;

            RolloverContainerRegistryCredentialsRequest() :
                PlayFabRequestCommon(),
                CustomTags()
            {}

            RolloverContainerRegistryCredentialsRequest(const RolloverContainerRegistryCredentialsRequest& src) :
                PlayFabRequestCommon(),
                CustomTags(src.CustomTags)
            {}

            ~RolloverContainerRegistryCredentialsRequest() = default;

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

        struct RolloverContainerRegistryCredentialsResponse : public PlayFabResultCommon
        {
            std::string DnsName;
            std::string Password;
            std::string Username;

            RolloverContainerRegistryCredentialsResponse() :
                PlayFabResultCommon(),
                DnsName(),
                Password(),
                Username()
            {}

            RolloverContainerRegistryCredentialsResponse(const RolloverContainerRegistryCredentialsResponse& src) :
                PlayFabResultCommon(),
                DnsName(src.DnsName),
                Password(src.Password),
                Username(src.Username)
            {}

            ~RolloverContainerRegistryCredentialsResponse() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["DnsName"], DnsName);
                FromJsonUtilS(input["Password"], Password);
                FromJsonUtilS(input["Username"], Username);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_DnsName; ToJsonUtilS(DnsName, each_DnsName); output["DnsName"] = each_DnsName;
                Json::Value each_Password; ToJsonUtilS(Password, each_Password); output["Password"] = each_Password;
                Json::Value each_Username; ToJsonUtilS(Username, each_Username); output["Username"] = each_Username;
                return output;
            }
        };

        struct ShutdownMultiplayerServerRequest : public PlayFabRequestCommon
        {
            std::map<std::string, std::string> CustomTags;
            std::string SessionId;

            ShutdownMultiplayerServerRequest() :
                PlayFabRequestCommon(),
                CustomTags(),
                SessionId()
            {}

            ShutdownMultiplayerServerRequest(const ShutdownMultiplayerServerRequest& src) :
                PlayFabRequestCommon(),
                CustomTags(src.CustomTags),
                SessionId(src.SessionId)
            {}

            ~ShutdownMultiplayerServerRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["CustomTags"], CustomTags);
                FromJsonUtilS(input["SessionId"], SessionId);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_CustomTags; ToJsonUtilS(CustomTags, each_CustomTags); output["CustomTags"] = each_CustomTags;
                Json::Value each_SessionId; ToJsonUtilS(SessionId, each_SessionId); output["SessionId"] = each_SessionId;
                return output;
            }
        };

        struct UntagContainerImageRequest : public PlayFabRequestCommon
        {
            std::map<std::string, std::string> CustomTags;
            std::string ImageName;
            std::string Tag;

            UntagContainerImageRequest() :
                PlayFabRequestCommon(),
                CustomTags(),
                ImageName(),
                Tag()
            {}

            UntagContainerImageRequest(const UntagContainerImageRequest& src) :
                PlayFabRequestCommon(),
                CustomTags(src.CustomTags),
                ImageName(src.ImageName),
                Tag(src.Tag)
            {}

            ~UntagContainerImageRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["CustomTags"], CustomTags);
                FromJsonUtilS(input["ImageName"], ImageName);
                FromJsonUtilS(input["Tag"], Tag);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_CustomTags; ToJsonUtilS(CustomTags, each_CustomTags); output["CustomTags"] = each_CustomTags;
                Json::Value each_ImageName; ToJsonUtilS(ImageName, each_ImageName); output["ImageName"] = each_ImageName;
                Json::Value each_Tag; ToJsonUtilS(Tag, each_Tag); output["Tag"] = each_Tag;
                return output;
            }
        };

        struct UpdateBuildAliasRequest : public PlayFabRequestCommon
        {
            std::string AliasId;
            std::string AliasName;
            std::list<BuildSelectionCriterion> BuildSelectionCriteria;
            std::map<std::string, std::string> CustomTags;

            UpdateBuildAliasRequest() :
                PlayFabRequestCommon(),
                AliasId(),
                AliasName(),
                BuildSelectionCriteria(),
                CustomTags()
            {}

            UpdateBuildAliasRequest(const UpdateBuildAliasRequest& src) :
                PlayFabRequestCommon(),
                AliasId(src.AliasId),
                AliasName(src.AliasName),
                BuildSelectionCriteria(src.BuildSelectionCriteria),
                CustomTags(src.CustomTags)
            {}

            ~UpdateBuildAliasRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["AliasId"], AliasId);
                FromJsonUtilS(input["AliasName"], AliasName);
                FromJsonUtilO(input["BuildSelectionCriteria"], BuildSelectionCriteria);
                FromJsonUtilS(input["CustomTags"], CustomTags);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_AliasId; ToJsonUtilS(AliasId, each_AliasId); output["AliasId"] = each_AliasId;
                Json::Value each_AliasName; ToJsonUtilS(AliasName, each_AliasName); output["AliasName"] = each_AliasName;
                Json::Value each_BuildSelectionCriteria; ToJsonUtilO(BuildSelectionCriteria, each_BuildSelectionCriteria); output["BuildSelectionCriteria"] = each_BuildSelectionCriteria;
                Json::Value each_CustomTags; ToJsonUtilS(CustomTags, each_CustomTags); output["CustomTags"] = each_CustomTags;
                return output;
            }
        };

        struct UpdateBuildNameRequest : public PlayFabRequestCommon
        {
            std::string BuildId;
            std::string BuildName;
            std::map<std::string, std::string> CustomTags;

            UpdateBuildNameRequest() :
                PlayFabRequestCommon(),
                BuildId(),
                BuildName(),
                CustomTags()
            {}

            UpdateBuildNameRequest(const UpdateBuildNameRequest& src) :
                PlayFabRequestCommon(),
                BuildId(src.BuildId),
                BuildName(src.BuildName),
                CustomTags(src.CustomTags)
            {}

            ~UpdateBuildNameRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["BuildId"], BuildId);
                FromJsonUtilS(input["BuildName"], BuildName);
                FromJsonUtilS(input["CustomTags"], CustomTags);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_BuildId; ToJsonUtilS(BuildId, each_BuildId); output["BuildId"] = each_BuildId;
                Json::Value each_BuildName; ToJsonUtilS(BuildName, each_BuildName); output["BuildName"] = each_BuildName;
                Json::Value each_CustomTags; ToJsonUtilS(CustomTags, each_CustomTags); output["CustomTags"] = each_CustomTags;
                return output;
            }
        };

        struct UpdateBuildRegionRequest : public PlayFabRequestCommon
        {
            std::string BuildId;
            BuildRegionParams BuildRegion;
            std::map<std::string, std::string> CustomTags;

            UpdateBuildRegionRequest() :
                PlayFabRequestCommon(),
                BuildId(),
                BuildRegion(),
                CustomTags()
            {}

            UpdateBuildRegionRequest(const UpdateBuildRegionRequest& src) :
                PlayFabRequestCommon(),
                BuildId(src.BuildId),
                BuildRegion(src.BuildRegion),
                CustomTags(src.CustomTags)
            {}

            ~UpdateBuildRegionRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["BuildId"], BuildId);
                FromJsonUtilO(input["BuildRegion"], BuildRegion);
                FromJsonUtilS(input["CustomTags"], CustomTags);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_BuildId; ToJsonUtilS(BuildId, each_BuildId); output["BuildId"] = each_BuildId;
                Json::Value each_BuildRegion; ToJsonUtilO(BuildRegion, each_BuildRegion); output["BuildRegion"] = each_BuildRegion;
                Json::Value each_CustomTags; ToJsonUtilS(CustomTags, each_CustomTags); output["CustomTags"] = each_CustomTags;
                return output;
            }
        };

        struct UpdateBuildRegionsRequest : public PlayFabRequestCommon
        {
            std::string BuildId;
            std::list<BuildRegionParams> BuildRegions;
            std::map<std::string, std::string> CustomTags;

            UpdateBuildRegionsRequest() :
                PlayFabRequestCommon(),
                BuildId(),
                BuildRegions(),
                CustomTags()
            {}

            UpdateBuildRegionsRequest(const UpdateBuildRegionsRequest& src) :
                PlayFabRequestCommon(),
                BuildId(src.BuildId),
                BuildRegions(src.BuildRegions),
                CustomTags(src.CustomTags)
            {}

            ~UpdateBuildRegionsRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["BuildId"], BuildId);
                FromJsonUtilO(input["BuildRegions"], BuildRegions);
                FromJsonUtilS(input["CustomTags"], CustomTags);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_BuildId; ToJsonUtilS(BuildId, each_BuildId); output["BuildId"] = each_BuildId;
                Json::Value each_BuildRegions; ToJsonUtilO(BuildRegions, each_BuildRegions); output["BuildRegions"] = each_BuildRegions;
                Json::Value each_CustomTags; ToJsonUtilS(CustomTags, each_CustomTags); output["CustomTags"] = each_CustomTags;
                return output;
            }
        };

        struct UploadCertificateRequest : public PlayFabRequestCommon
        {
            std::map<std::string, std::string> CustomTags;
            Certificate GameCertificate;

            UploadCertificateRequest() :
                PlayFabRequestCommon(),
                CustomTags(),
                GameCertificate()
            {}

            UploadCertificateRequest(const UploadCertificateRequest& src) :
                PlayFabRequestCommon(),
                CustomTags(src.CustomTags),
                GameCertificate(src.GameCertificate)
            {}

            ~UploadCertificateRequest() = default;

            void FromJson(const Json::Value& input) override
            {
                FromJsonUtilS(input["CustomTags"], CustomTags);
                FromJsonUtilO(input["GameCertificate"], GameCertificate);
            }

            Json::Value ToJson() const override
            {
                Json::Value output;
                Json::Value each_CustomTags; ToJsonUtilS(CustomTags, each_CustomTags); output["CustomTags"] = each_CustomTags;
                Json::Value each_GameCertificate; ToJsonUtilO(GameCertificate, each_GameCertificate); output["GameCertificate"] = each_GameCertificate;
                return output;
            }
        };

    }
}

#endif
