// SampleSpecificAssets.inl
//
// Asset definitions for use by the Game Save sample.
//
// Xbox Advanced Technology Group (ATG)
// Copyright (c) Microsoft Corporation. All rights reserved.
//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//--------------------------------------------------------------------------------------

#if defined(GENERATE_DESCRIPTOR_ENUM)

// Each asset must have a unique value in this table.
//
// NOTE: The last value in the enum class AssetDescriptor : size_t enum should be the count of elements, 
//       called DESCRIPTOR_COUNT

enum class AssetDescriptor : size_t
{
   SegoeUI24Font,
   SegoeUILight42Font,
   SegoeUISemilight18Font,
   SegoeUISemilight42Font,
   Consolas12Font,
   Consolas16Font,
   Blank,                 // Used for screen backgrounds
   TitleLogo,
   GameBoardBackground,
   Cursor,
   HorizontalWordLinker,
   VerticalWordLinker,
   ControllerXButtonIcon,
   ControllerLeftStickIcon,
   ControllerLeftBumperIcon,
   ControllerRightStickIcon,
   ControllerRightBumperIcon,
   ControllerDPadIcon,
   SaveSlot1Glyph,
   SaveSlot2Glyph,
   SaveSlot3Glyph,
   SaveSlot4Glyph,
   SaveSlot5Glyph,
   SaveSlot6Glyph,
   SaveSlot7Glyph,
   SaveSlot8Glyph,
   SaveSlot9Glyph,
   SaveSlot1GlyphHighlighted,
   SaveSlot2GlyphHighlighted,
   SaveSlot3GlyphHighlighted,
   SaveSlot4GlyphHighlighted,
   SaveSlot5GlyphHighlighted,
   SaveSlot6GlyphHighlighted,
   SaveSlot7GlyphHighlighted,
   SaveSlot8GlyphHighlighted,
   SaveSlot9GlyphHighlighted,

   // This is not a valid value, and is used as a count of descriptors for creating the descriptor heaps.
   DESCRIPTOR_COUNT
};

#elif defined(GENERATE_ASSET_TABLE)

   // These are all of the assets used by the game, with their matching asset type and descriptor index (which should always
   // match their index in this table). Normally you would want an editor to generate these for you, and more control over
   // which assets are loaded when, refcounts, etc. - adding this functionality is left as an exercise for the reader.

   // These are set up as follows:
   // Column 1 - The descriptor that will be used to identify this asset by the DX12 resource heaps (see enum above).
   // Column 2 - The path to the asset on disk.
   // Column 3 - The type of the asset. We currently only support DXTK SpriteFonts, and Textures.

ATG::AssetEntry AssetTable[] =
{
   { AssetDescriptor::SegoeUI24Font,            L"Assets\\Fonts\\SegoeUI_24_NP.spritefont",          ATG::AssetType::SpriteFont },
   { AssetDescriptor::SegoeUILight42Font,       L"Assets\\Fonts\\SegoeUILight_42_NP.spritefont",     ATG::AssetType::SpriteFont },
   { AssetDescriptor::SegoeUISemilight18Font,   L"Assets\\Fonts\\SegoeUISemilight_18_NP.spritefont", ATG::AssetType::SpriteFont },
   { AssetDescriptor::SegoeUISemilight42Font,   L"Assets\\Fonts\\SegoeUISemilight_42_NP.spritefont", ATG::AssetType::SpriteFont },
   { AssetDescriptor::Consolas12Font,           L"Assets\\Fonts\\Consolas_12_NP.spritefont",         ATG::AssetType::SpriteFont },
   { AssetDescriptor::Consolas16Font,           L"Assets\\Fonts\\Consolas_12_NP.spritefont",         ATG::AssetType::SpriteFont },
   { AssetDescriptor::Blank,                    L"Assets\\blank.png",                                ATG::AssetType::Texture },
   { AssetDescriptor::TitleLogo,                L"Assets\\WordGame_logo.png",                        ATG::AssetType::Texture },
   { AssetDescriptor::GameBoardBackground,      L"Assets\\Board\\BOARD_BG_PC.jpg",                   ATG::AssetType::Texture },
   { AssetDescriptor::Cursor,                   L"Assets\\Board\\gameboard_letter_ON.png",           ATG::AssetType::Texture },
   { AssetDescriptor::HorizontalWordLinker,     L"Assets\\Board\\wordformed_arrow_horiz.png",        ATG::AssetType::Texture },
   { AssetDescriptor::VerticalWordLinker,       L"Assets\\Board\\wordformed_arrow_vert.png",         ATG::AssetType::Texture },
   { AssetDescriptor::ControllerXButtonIcon,    L"Assets\\Controller\\x.png",                        ATG::AssetType::Texture },
   { AssetDescriptor::ControllerLeftStickIcon,  L"Assets\\Controller\\ls.png",                       ATG::AssetType::Texture },
   { AssetDescriptor::ControllerLeftBumperIcon, L"Assets\\Controller\\lb.png",                       ATG::AssetType::Texture },
   { AssetDescriptor::ControllerRightStickIcon, L"Assets\\Controller\\rs.png",                       ATG::AssetType::Texture },
   { AssetDescriptor::ControllerRightBumperIcon, L"Assets\\Controller\\rb.png",                      ATG::AssetType::Texture },
   { AssetDescriptor::ControllerDPadIcon,       L"Assets\\Controller\\dpad.png",                     ATG::AssetType::Texture },
   { AssetDescriptor::SaveSlot1Glyph,           L"Assets\\Buttons\\btnSaveSlot01.png",               ATG::AssetType::Texture },
   { AssetDescriptor::SaveSlot2Glyph,           L"Assets\\Buttons\\btnSaveSlot02.png",               ATG::AssetType::Texture },
   { AssetDescriptor::SaveSlot3Glyph,           L"Assets\\Buttons\\btnSaveSlot03.png",               ATG::AssetType::Texture },
   { AssetDescriptor::SaveSlot4Glyph,           L"Assets\\Buttons\\btnSaveSlot04.png",               ATG::AssetType::Texture },
   { AssetDescriptor::SaveSlot5Glyph,           L"Assets\\Buttons\\btnSaveSlot05.png",               ATG::AssetType::Texture },
   { AssetDescriptor::SaveSlot6Glyph,           L"Assets\\Buttons\\btnSaveSlot06.png",               ATG::AssetType::Texture },
   { AssetDescriptor::SaveSlot7Glyph,           L"Assets\\Buttons\\btnSaveSlot07.png",               ATG::AssetType::Texture },
   { AssetDescriptor::SaveSlot8Glyph,           L"Assets\\Buttons\\btnSaveSlot08.png",               ATG::AssetType::Texture },
   { AssetDescriptor::SaveSlot9Glyph,           L"Assets\\Buttons\\btnSaveSlot09.png",               ATG::AssetType::Texture },
   { AssetDescriptor::SaveSlot1GlyphHighlighted, L"Assets\\Buttons\\btnSaveSlot01_ON.png",           ATG::AssetType::Texture },
   { AssetDescriptor::SaveSlot2GlyphHighlighted, L"Assets\\Buttons\\btnSaveSlot02_ON.png",           ATG::AssetType::Texture },
   { AssetDescriptor::SaveSlot3GlyphHighlighted, L"Assets\\Buttons\\btnSaveSlot03_ON.png",           ATG::AssetType::Texture },
   { AssetDescriptor::SaveSlot4GlyphHighlighted, L"Assets\\Buttons\\btnSaveSlot04_ON.png",           ATG::AssetType::Texture },
   { AssetDescriptor::SaveSlot5GlyphHighlighted, L"Assets\\Buttons\\btnSaveSlot05_ON.png",           ATG::AssetType::Texture },
   { AssetDescriptor::SaveSlot6GlyphHighlighted, L"Assets\\Buttons\\btnSaveSlot06_ON.png",           ATG::AssetType::Texture },
   { AssetDescriptor::SaveSlot7GlyphHighlighted, L"Assets\\Buttons\\btnSaveSlot07_ON.png",           ATG::AssetType::Texture },
   { AssetDescriptor::SaveSlot8GlyphHighlighted, L"Assets\\Buttons\\btnSaveSlot08_ON.png",           ATG::AssetType::Texture },
   { AssetDescriptor::SaveSlot9GlyphHighlighted, L"Assets\\Buttons\\btnSaveSlot09_ON.png",           ATG::AssetType::Texture }
};

#else
#error Asset definition file #included without defining GENERATE_ASSET_TABLE or GENERATE_DESCRIPTOR_ENUM.
#endif
