// Copyright 2023 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include "common/common_types.h"

constexpr u32 SYSTEM_FIRMWARE_UPPER_TITLE_ID = 0x00040138;
constexpr u32 SYSTEM_APPLICATION_UPPER_TITLE_ID = 0x00040010;
constexpr u32 SYSTEM_DATA_ARCHIVE_UPPER_TITLE_ID = 0x0004001B;
constexpr u32 SYSTEM_APPLET_UPPER_TITLE_ID = 0x00040030;
constexpr u32 SHARED_DATA_ARCHIVE_UPPER_TITLE_ID = 0x0004009B;
constexpr u32 SYSTEM_DATA_ARCHIVE_2_UPPER_TITLE_ID = 0x000400DB;
constexpr u32 SYSTEM_MODULE_UPPER_TITLE_ID = 0x00040130;

struct Title {
    enum Mode { All, Recommended, Minimal };
    std::string name;
    u32 upper_title_id;
    std::array<u32, 6> lower_title_id;
    Mode mode = Mode::All;
};

static const std::array<Title, 9> SYSTEM_FIRMWARE = {
    {{"Safe Mode Native Firmware",
      SYSTEM_FIRMWARE_UPPER_TITLE_ID,
      {{0x00000003, 0x00000003, 0x00000003, 0x00000003, 0x00000003, 0x00000003}},
      Title::Mode::Minimal},
     {"New_3DS Safe Mode Native Firmware",
      SYSTEM_FIRMWARE_UPPER_TITLE_ID,
      {{0x20000003, 0x20000003, 0x20000003, 0x20000003, 0x20000003, 0x20000003}},
      Title::Mode::Minimal},
     {"Native Firmware",
      SYSTEM_FIRMWARE_UPPER_TITLE_ID,
      {{0x00000002, 0x00000002, 0x00000002, 0x00000002, 0x00000002, 0x00000002}},
      Title::Mode::Minimal},
     {"New_3DS Native Firmware",
      SYSTEM_FIRMWARE_UPPER_TITLE_ID,
      {{0x20000002, 0x20000002, 0x20000002, 0x20000002, 0x20000002, 0x20000002}},
      Title::Mode::Minimal}}};
static const std::array<Title, 17> SYSTEM_APPLICATIONS = {
    {{"System Settings",
      SYSTEM_APPLICATION_UPPER_TITLE_ID,
      {{0x00020000, 0x00021000, 0x00022000, 0x00026000, 0x00027000, 0x00028000}},
      Title::Mode::All},
     {"Download Play",
      SYSTEM_APPLICATION_UPPER_TITLE_ID,
      {{0x00020100, 0x00021100, 0x00022100, 0x00026100, 0x00027100, 0x00028100}},
      Title::Mode::Recommended},
     {"Activity Log",
      SYSTEM_APPLICATION_UPPER_TITLE_ID,
      {{0x00020200, 0x00021200, 0x00022200, 0x00026200, 0x00027200, 0x00028200}},
      Title::Mode::All},
     {"Health and Safety Information",
      SYSTEM_APPLICATION_UPPER_TITLE_ID,
      {{0x00020300, 0x00021300, 0x00022300, 0x00026300, 0x00027300, 0x00028300}},
      Title::Mode::All},
     {"New_3DS Health and Safety Information",
      SYSTEM_APPLICATION_UPPER_TITLE_ID,
      {{0x20020300, 0x20021300, 0x20022300, 0x0, 0x20027300, 0x0}},
      Title::Mode::All},
     {"Nintendo 3DS Camera",
      SYSTEM_APPLICATION_UPPER_TITLE_ID,
      {{0x00020400, 0x00021400, 0x00022400, 0x00026400, 0x00027400, 0x00028400}},
      Title::Mode::All},
     {"Nintendo 3DS Sound",
      SYSTEM_APPLICATION_UPPER_TITLE_ID,
      {{0x00020500, 0x00021500, 0x00022500, 0x00026500, 0x00027500, 0x00028500}},
      Title::Mode::All},
     {"Mii Maker",
      SYSTEM_APPLICATION_UPPER_TITLE_ID,
      {{0x00020700, 0x00021700, 0x00022700, 0x00026700, 0x00027700, 0x00028700}},
      Title::Mode::Recommended},
     {"StreetPass Mii Plaza",
      SYSTEM_APPLICATION_UPPER_TITLE_ID,
      {{0x00020800, 0x00021800, 0x00022800, 0x00026800, 0x00027800, 0x00028800}},
      Title::Mode::All},
     {"eShop",
      SYSTEM_APPLICATION_UPPER_TITLE_ID,
      {{0x00020900, 0x00021900, 0x00022900, 0x0, 0x00027900, 0x00028900}},
      Title::Mode::Recommended},
     {"System Transfer",
      SYSTEM_APPLICATION_UPPER_TITLE_ID,
      {{0x00020A00, 0x00021A00, 0x00022A00, 0x0, 0x00027A00, 0x00028A00}},
      Title::Mode::All},
     {"Nintendo Zone",
      SYSTEM_APPLICATION_UPPER_TITLE_ID,
      {{0x00020B00, 0x00021B00, 0x00022B00, 0x0, 0x0, 0x0}},
      Title::Mode::All},
     {"Face Raiders",
      SYSTEM_APPLICATION_UPPER_TITLE_ID,
      {{0x00020D00, 0x00021D00, 0x00022D00, 0x00026D00, 0x00027D00, 0x00028D00}},
      Title::Mode::All},
     {"New_3DS Face Raiders",
      SYSTEM_APPLICATION_UPPER_TITLE_ID,
      {{0x20020D00, 0x20021D00, 0x20022D00, 0x0, 0x20027D00, 0x0}},
      Title::Mode::All},
     {"AR Games",
      SYSTEM_APPLICATION_UPPER_TITLE_ID,
      {{0x00020E00, 0x00021E00, 0x00022E00, 0x00026E00, 0x00027E00, 0x00028E00}},
      Title::Mode::All},
     {"Nintendo Network ID Settings",
      SYSTEM_APPLICATION_UPPER_TITLE_ID,
      {{0x0002BF00, 0x0002C000, 0x0002C100, 0x0, 0x0, 0x0}},
      Title::Mode::Recommended},
     {"microSD Management",
      SYSTEM_APPLICATION_UPPER_TITLE_ID,
      {{0x20023100, 0x20024100, 0x20025100, 0x0, 0x0, 0x0}},
      Title::Mode::All}}};

static const std::array<Title, 7> SYSTEM_DATA_ARCHIVES = {
    {{"ClCertA",
      SYSTEM_DATA_ARCHIVE_UPPER_TITLE_ID,
      {{0x00010002, 0x00010002, 0x00010002, 0x00010002, 0x00010002, 0x00010002}},
      Title::Mode::Recommended},
     {"NS CFA",
      SYSTEM_DATA_ARCHIVE_UPPER_TITLE_ID,
      {{0x00010702, 0x00010702, 0x00010702, 0x00010702, 0x00010702, 0x00010702}},
      Title::Mode::All},
     {"dummy.txt",
      SYSTEM_DATA_ARCHIVE_UPPER_TITLE_ID,
      {{0x00010802, 0x00010802, 0x00010802, 0x00010802, 0x00010802, 0x00010802}},
      Title::Mode::All},
     {"CFA web-browser data",
      SYSTEM_DATA_ARCHIVE_UPPER_TITLE_ID,
      {{0x00018002, 0x00018002, 0x00018002, 0x00018002, 0x00018002, 0x00018002}},
      Title::Mode::All},
     {"local web-browser data",
      SYSTEM_DATA_ARCHIVE_UPPER_TITLE_ID,
      {{0x00018102, 0x00018102, 0x00018102, 0x00018102, 0x00018102, 0x00018102}},
      Title::Mode::All},
     {"webkit/OSS CROs",
      SYSTEM_DATA_ARCHIVE_UPPER_TITLE_ID,
      {{0x00018202, 0x00018202, 0x00018202, 0x00018202, 0x00018202, 0x00018202}},
      Title::Mode::All},
     {"Fangate_updater",
      SYSTEM_DATA_ARCHIVE_UPPER_TITLE_ID,
      {{0x00019002, 0x00019002, 0x00019002, 0x00019002, 0x00019002, 0x00019002}},
      Title::Mode::All}}};

static const std::array<Title, 27> SYSTEM_APPLETS = {
    {{"Home Menu",
      SYSTEM_APPLET_UPPER_TITLE_ID,
      {{0x00008202, 0x00008F02, 0x00009802, 0x0000A102, 0x0000A902, 0x0000B102}},
      Title::Mode::All},
     {"Camera applet",
      SYSTEM_APPLET_UPPER_TITLE_ID,
      {{0x00008402, 0x00009002, 0x00009902, 0x0000A202, 0x0000AA02, 0x0000B202}},
      Title::Mode::All},
     {"Instruction Manual",
      SYSTEM_APPLET_UPPER_TITLE_ID,
      {{0x00008602, 0x00009202, 0x00009B02, 0x0000A402, 0x0000AC02, 0x0000B402}},
      Title::Mode::Recommended},
     {"Game Notes",
      SYSTEM_APPLET_UPPER_TITLE_ID,
      {{0x00008702, 0x00009302, 0x00009C02, 0x0000A502, 0x0000AD02, 0x0000B502}},
      Title::Mode::All},
     {"Internet Browser",
      SYSTEM_APPLET_UPPER_TITLE_ID,
      {{0x00008802, 0x00009402, 0x00009D02, 0x0000A602, 0x0000AE02, 0x0000B602}},
      Title::Mode::All},
     {"New 3DS Internet Browser",
      SYSTEM_APPLET_UPPER_TITLE_ID,
      {{0x20008802, 0x20009402, 0x20009D02, 0x0, 0x2000AE02, 0x0}},
      Title::Mode::All},
     {"Fatal error viewer",
      SYSTEM_APPLET_UPPER_TITLE_ID,
      {{0x00008A02, 0x00008A02, 0x00008A02, 0x00008A02, 0x00008A02, 0x00008A02}},
      Title::Mode::All},
     {"Safe Mode Fatal error viewer",
      SYSTEM_APPLET_UPPER_TITLE_ID,
      {{0x00008A03, 0x00008A03, 0x00008A03, 0x00008A03, 0x00008A03, 0x00008A03}},
      Title::Mode::All},
     {"New 3DS Safe Mode Fatal error viewer",
      SYSTEM_APPLET_UPPER_TITLE_ID,
      {{0x20008A03, 0x20008A03, 0x20008A03, 0x0, 0x20008A03, 0x0}},
      Title::Mode::All},
     {"Friend List",
      SYSTEM_APPLET_UPPER_TITLE_ID,
      {{0x00008D02, 0x00009602, 0x00009F02, 0x0000A702, 0x0000AF02, 0x0000B702}},
      Title::Mode::Recommended},
     {"Notifications",
      SYSTEM_APPLET_UPPER_TITLE_ID,
      {{0x00008E02, 0x000009702, 0x0000A002, 0x0000A802, 0x0000B002, 0x0000B802}},
      Title::Mode::All},
     {"Software Keyboard",
      SYSTEM_APPLET_UPPER_TITLE_ID,
      {{0x00000C002, 0x0000C802, 0x0000D002, 0x0000D802, 0x0000DE02, 0x0000E402}},
      Title::Mode::Recommended},
     {"Safe Mode Software Keyboard",
      SYSTEM_APPLET_UPPER_TITLE_ID,
      {{0x00000C003, 0x0000C803, 0x0000D003, 0x0000D803, 0x0000DE03, 0x0000E403}},
      Title::Mode::All},
     {"New 3DS Safe Mode Software Keyboard",
      SYSTEM_APPLET_UPPER_TITLE_ID,
      {{0x2000C003, 0x2000C803, 0x2000D003, 0x0, 0x2000DE03, 0x0}},
      Title::Mode::All},
     {"Mii picker",
      SYSTEM_APPLET_UPPER_TITLE_ID,
      {{0x0000C102, 0x0000C902, 0x0000D102, 0x0000D902, 0x0000DF02, 0x0000E502}},
      Title::Mode::Recommended},
     {"Picture picker",
      SYSTEM_APPLET_UPPER_TITLE_ID,
      {{0x0000C302, 0x0000CB02, 0x0000D302, 0x0000DB02, 0x0000E102, 0x0000E702}},
      Title::Mode::All},
     {"Voice memo picker",
      SYSTEM_APPLET_UPPER_TITLE_ID,
      {{0x0000C402, 0x0000CC02, 0x0000D402, 0x0000DC02, 0x0000E202, 0x0000E802}},
      Title::Mode::All},
     {"Error display",
      SYSTEM_APPLET_UPPER_TITLE_ID,
      {{0x0000C502, 0x0000C502, 0x0000C502, 0x0000CF02, 0x0000CF02, 0x0000CF02}},
      Title::Mode::All},
     {"Safe mode error display",
      SYSTEM_APPLET_UPPER_TITLE_ID,
      {{0x0000C503, 0x0000C503, 0x0000C503, 0x0000CF03, 0x0000CF03, 0x0000CF03}},
      Title::Mode::All},
     {"New 3DS safe mode error display",
      SYSTEM_APPLET_UPPER_TITLE_ID,
      {{0x2000C503, 0x2000C503, 0x2000C503, 0x0, 0x2000CF03, 0x0}},
      Title::Mode::All},
     {"Circle Pad Pro test/calibration applet",
      SYSTEM_APPLET_UPPER_TITLE_ID,
      {{0x0000CD02, 0x0000CD02, 0x0000CD02, 0x0000D502, 0x0000D502, 0x0000D502}},
      Title::Mode::All},
     {"eShop applet",
      SYSTEM_APPLET_UPPER_TITLE_ID,
      {{0x0000C602, 0x0000CE02, 0x0000D602, 0x0, 0x0000E302, 0x0000E902}},
      Title::Mode::Recommended},
     {"Miiverse",
      SYSTEM_APPLET_UPPER_TITLE_ID,
      {{0x0000BC02, 0x0000BC02, 0x0000BC02, 0x0, 0x0, 0x0}},
      Title::Mode::All},
     {"Miiverse system library",
      SYSTEM_APPLET_UPPER_TITLE_ID,
      {{0x0000F602, 0x0000F602, 0x0000F602, 0x0, 0x0, 0x0}},
      Title::Mode::All},
     {"Miiverse-posting applet",
      SYSTEM_APPLET_UPPER_TITLE_ID,
      {{0x00008302, 0x00008B02, 0x0000BA02, 0x0, 0x0, 0x0}},
      Title::Mode::All},
     {"Amiibo Settings",
      SYSTEM_APPLET_UPPER_TITLE_ID,
      {{0x00009502, 0x00009E02, 0x0000B902, 0x0, 0x00008C02, 0x0000BF02}},
      Title::Mode::All}}};

static const std::array<Title, 25> SHARED_DATA_ARCHIVES = {
    {{"CFL_Res.dat",
      SHARED_DATA_ARCHIVE_UPPER_TITLE_ID,
      {{0x00010202, 0x00010202, 0x00010202, 0x00010202, 0x00010202, 0x00010202}},
      Title::Mode::All},
     {"Region Manifest",
      SHARED_DATA_ARCHIVE_UPPER_TITLE_ID,
      {{0x00010402, 0x00010402, 0x00010402, 0x00010402, 0x00010402, 0x00010402}},
      Title::Mode::All},
     {"Non-Nintendo TLS Root-CA Certificates",
      SHARED_DATA_ARCHIVE_UPPER_TITLE_ID,
      {{0x00010602, 0x00010602, 0x00010602, 0x00010602, 0x00010602, 0x00010602}},
      Title::Mode::Recommended},
     {"CHN/CN Dictionary",
      SHARED_DATA_ARCHIVE_UPPER_TITLE_ID,
      {{0x0, 0x0, 0x0, 0x00011002, 0x0, 0x0}},
      Title::Mode::All},
     {"TWN/TN dictionary",
      SHARED_DATA_ARCHIVE_UPPER_TITLE_ID,
      {{0x0, 0x0, 0x0, 0x0, 0x0, 0x00011102}},
      Title::Mode::All},
     {"NL/NL dictionary",
      SHARED_DATA_ARCHIVE_UPPER_TITLE_ID,
      {{0x0, 0x0, 0x00011202, 0x0, 0x0, 0x0}},
      Title::Mode::All},
     {"EN/GB dictionary",
      SHARED_DATA_ARCHIVE_UPPER_TITLE_ID,
      {{0x0, 0x0, 0x00011302, 0x0, 0x0, 0x0}},
      Title::Mode::All},
     {"EN/US dictionary",
      SHARED_DATA_ARCHIVE_UPPER_TITLE_ID,
      {{0x0, 0x00011402, 0x0, 0x0, 0x0, 0x0}},
      Title::Mode::All},
     {"FR/FR/regular dictionary",
      SHARED_DATA_ARCHIVE_UPPER_TITLE_ID,
      {{0x0, 0x0, 0x00011502, 0x0, 0x0, 0x0}},
      Title::Mode::All},
     {"FR/CA/regular dictionary",
      SHARED_DATA_ARCHIVE_UPPER_TITLE_ID,
      {{0x0, 0x00011602, 0x0, 0x0, 0x0, 0x0}},
      Title::Mode::All},
     {"DE/regular dictionary",
      SHARED_DATA_ARCHIVE_UPPER_TITLE_ID,
      {{0x0, 0x0, 0x00011702, 0x0, 0x0, 0x0}},
      Title::Mode::All},
     {"IT/IT dictionary",
      SHARED_DATA_ARCHIVE_UPPER_TITLE_ID,
      {{0x0, 0x0, 0x00011802, 0x0, 0x0, 0x0}},
      Title::Mode::All},
     {"JA_small/32 dictionary",
      SHARED_DATA_ARCHIVE_UPPER_TITLE_ID,
      {{0x00011902, 0x0, 0x0, 0x0, 0x0, 0x0}},
      Title::Mode::All},
     {"KO/KO dictionary",
      SHARED_DATA_ARCHIVE_UPPER_TITLE_ID,
      {{0x0, 0x0, 0x0, 0x0, 0x00011A02, 0x0}},
      Title::Mode::All},
     {"PT/PT/regular dictionary",
      SHARED_DATA_ARCHIVE_UPPER_TITLE_ID,
      {{0x0, 0x0, 0x00011B02, 0x0, 0x0, 0x0}},
      Title::Mode::All},
     {"RU/regular dictionary",
      SHARED_DATA_ARCHIVE_UPPER_TITLE_ID,
      {{0x0, 0x0, 0x00011C02, 0x0, 0x0, 0x0}},
      Title::Mode::All},
     {"ES/ES dictionary",
      SHARED_DATA_ARCHIVE_UPPER_TITLE_ID,
      {{0x0, 0x00011D02, 0x00011D02, 0x0, 0x0, 0x0}},
      Title::Mode::All},
     {"PT/BR/regular dictionary",
      SHARED_DATA_ARCHIVE_UPPER_TITLE_ID,
      {{0x0, 0x00011E02, 0x0, 0x0, 0x0, 0x0}},
      Title::Mode::All},
     {"error strings",
      SHARED_DATA_ARCHIVE_UPPER_TITLE_ID,
      {{0x00012202, 0x00012302, 0x00012102, 0x00012402, 0x00012502, 0x00012602}},
      Title::Mode::All},
     {"eula",
      SHARED_DATA_ARCHIVE_UPPER_TITLE_ID,
      {{0x00013202, 0x00013302, 0x00013102, 0x00013502, 0x0, 0x0}},
      Title::Mode::All},
     {"JPN/EUR/USA System Font",
      SHARED_DATA_ARCHIVE_UPPER_TITLE_ID,
      {{0x00014002, 0x00014002, 0x00014002, 0x00014002, 0x00014002, 0x00014002}},
      Title::Mode::Recommended},
     {"CHN System Font",
      SHARED_DATA_ARCHIVE_UPPER_TITLE_ID,
      {{0x00014102, 0x00014102, 0x00014102, 0x00014102, 0x00014102, 0x00014102}},
      Title::Mode::Recommended},
     {"KOR System Font",
      SHARED_DATA_ARCHIVE_UPPER_TITLE_ID,
      {{0x00014202, 0x00014202, 0x00014202, 0x00014202, 0x00014202, 0x00014202}},
      Title::Mode::Recommended},
     {"TWN System Font",
      SHARED_DATA_ARCHIVE_UPPER_TITLE_ID,
      {{0x00014302, 0x00014302, 0x00014302, 0x00014302, 0x00014302, 0x00014302}},
      Title::Mode::Recommended},
     {"rate",
      SHARED_DATA_ARCHIVE_UPPER_TITLE_ID,
      {{0x00015202, 0x00015302, 0x00015102, 0x0, 0x0015502, 0x00015602}},
      Title::Mode::All}}};

static const std::array<Title, 5> SYSTEM_DATA_ARCHIVES_2 = {
    {{"bad word list",
      SYSTEM_DATA_ARCHIVE_2_UPPER_TITLE_ID,
      {{0x00010302, 0x00010302, 0x00010302, 0x00010302, 0x00010302, 0x00010302}},
      Title::Mode::All},
     {"Nintendo Zone hotspot list",
      SYSTEM_DATA_ARCHIVE_2_UPPER_TITLE_ID,
      {{0x00010502, 0x00010502, 0x00010502, 0x00010502, 0x00010502, 0x00010502}},
      Title::Mode::All},
     {"NVer",
      SYSTEM_DATA_ARCHIVE_2_UPPER_TITLE_ID,
      {{0x00016102, 0x00016202, 0x00016302, 0x00016402, 0x00016502, 0x00016602}},
      Title::Mode::All},
     {"New_3DS NVer",
      SYSTEM_DATA_ARCHIVE_2_UPPER_TITLE_ID,
      {{0x20016102, 0x20016202, 0x20016302, 0x0, 0x20016502, 0x0}},
      Title::Mode::All},
     {"CVer",
      SYSTEM_DATA_ARCHIVE_2_UPPER_TITLE_ID,
      {{0x00017102, 0x00017202, 0x00017302, 0x00017402, 0x00017502, 0x00017602}},
      Title::Mode::All}}};

static const std::array<Title, 100> SYSTEM_MODULES = {
    {{"AM ( Application Manager )",
      SYSTEM_MODULE_UPPER_TITLE_ID,
      {{0x00001502, 0x00001502, 0x00001502, 0x00001502, 0x00001502, 0x00001502}},
      Title::Mode::All},
     {"Safe Mode AM",
      SYSTEM_MODULE_UPPER_TITLE_ID,
      {{0x00001503, 0x00001503, 0x00001503, 0x00001503, 0x00001503, 0x00001503}},
      Title::Mode::All},
     {"New_3DS Safe Mode AM",
      SYSTEM_MODULE_UPPER_TITLE_ID,
      {{0x20001503, 0x20001503, 0x20001503, 0x20001503, 0x20001503, 0x20001503}},
      Title::Mode::All},
     {"Camera",
      SYSTEM_MODULE_UPPER_TITLE_ID,
      {{0x00001602, 0x00001602, 0x00001602, 0x00001602, 0x00001602, 0x00001602}},
      Title::Mode::All},
     {"New_3DS Camera",
      SYSTEM_MODULE_UPPER_TITLE_ID,
      {{0x20001602, 0x20001602, 0x20001602, 0x20001602, 0x20001602, 0x20001602}},
      Title::Mode::All},
     {"Config (cfg)",
      SYSTEM_MODULE_UPPER_TITLE_ID,
      {{0x00001702, 0x00001702, 0x00001702, 0x00001702, 0x00001702, 0x00001702}},
      Title::Mode::All},
     {"Safe Mode Config (cfg)",
      SYSTEM_MODULE_UPPER_TITLE_ID,
      {{0x00001703, 0x00001703, 0x00001703, 0x00001703, 0x00001703, 0x00001703}},
      Title::Mode::All},
     {"New_3DS Safe Mode Config (cfg)",
      SYSTEM_MODULE_UPPER_TITLE_ID,
      {{0x20001703, 0x20001703, 0x20001703, 0x20001703, 0x20001703, 0x20001703}},
      Title::Mode::All},
     {"Codec",
      SYSTEM_MODULE_UPPER_TITLE_ID,
      {{0x00001802, 0x00001802, 0x00001802, 0x00001802, 0x00001802, 0x00001802}},
      Title::Mode::All},
     {"Safe Mode Codec",
      SYSTEM_MODULE_UPPER_TITLE_ID,
      {{0x00001803, 0x00001803, 0x00001803, 0x00001803, 0x00001803, 0x00001803}},
      Title::Mode::All},
     {"New_3DS Safe Mode Codec",
      SYSTEM_MODULE_UPPER_TITLE_ID,
      {{0x20001803, 0x20001803, 0x20001803, 0x20001803, 0x20001803, 0x20001803}},
      Title::Mode::All},
     {"DSP",
      SYSTEM_MODULE_UPPER_TITLE_ID,
      {{0x00001A02, 0x00001A02, 0x00001A02, 0x00001A02, 0x00001A02, 0x00001A02}},
      Title::Mode::All},
     {"Safe Mode DSP",
      SYSTEM_MODULE_UPPER_TITLE_ID,
      {{0x00001A03, 0x00001A03, 0x00001A03, 0x00001A03, 0x00001A03, 0x00001A03}},
      Title::Mode::All},
     {"New_3DS Safe Mode DSP",
      SYSTEM_MODULE_UPPER_TITLE_ID,
      {{0x20001A03, 0x20001A03, 0x20001A03, 0x20001A03, 0x20001A03, 0x20001A03}},
      Title::Mode::All},
     {"GPIO",
      SYSTEM_MODULE_UPPER_TITLE_ID,
      {{0x00001B02, 0x00001B02, 0x00001B02, 0x00001B02, 0x00001B02, 0x00001B02}},
      Title::Mode::All},
     {"Safe Mode GPIO",
      SYSTEM_MODULE_UPPER_TITLE_ID,
      {{0x00001B03, 0x00001B03, 0x00001B03, 0x00001B03, 0x00001B03, 0x00001B03}},
      Title::Mode::All},
     {"New_3DS Safe Mode GPIO",
      SYSTEM_MODULE_UPPER_TITLE_ID,
      {{0x20001B03, 0x20001B03, 0x20001B03, 0x20001B03, 0x20001B03, 0x20001B03}},
      Title::Mode::All},
     {"GSP",
      SYSTEM_MODULE_UPPER_TITLE_ID,
      {{0x00001C02, 0x00001C02, 0x00001C02, 0x00001C02, 0x00001C02, 0x00001C02}},
      Title::Mode::All},
     {"New_3DS GSP",
      SYSTEM_MODULE_UPPER_TITLE_ID,
      {{0x20001C02, 0x20001C02, 0x20001C02, 0x20001C02, 0x20001C02, 0x20001C02}},
      Title::Mode::All},
     {"Safe Mode GSP",
      SYSTEM_MODULE_UPPER_TITLE_ID,
      {{0x00001C03, 0x00001C03, 0x00001C03, 0x00001C03, 0x00001C03, 0x00001C03}},
      Title::Mode::All},
     {"New_3DS Safe Mode GSP",
      SYSTEM_MODULE_UPPER_TITLE_ID,
      {{0x20001C03, 0x20001C03, 0x20001C03, 0x20001C03, 0x20001C03, 0x20001C03}},
      Title::Mode::All},
     {"HID (Human Interface Devices)",
      SYSTEM_MODULE_UPPER_TITLE_ID,
      {{0x00001D02, 0x00001D02, 0x00001D02, 0x00001D02, 0x00001D02, 0x00001D02}},
      Title::Mode::All},
     {"Safe Mode HID",
      SYSTEM_MODULE_UPPER_TITLE_ID,
      {{0x00001D03, 0x00001D03, 0x00001D03, 0x00001D03, 0x00001D03, 0x00001D03}},
      Title::Mode::All},
     {"New_3DS Safe Mode HID",
      SYSTEM_MODULE_UPPER_TITLE_ID,
      {{0x20001D03, 0x20001D03, 0x20001D03, 0x20001D03, 0x20001D03, 0x20001D03}},
      Title::Mode::All},
     {"i2c",
      SYSTEM_MODULE_UPPER_TITLE_ID,
      {{0x00001E02, 0x00001E02, 0x00001E02, 0x00001E02, 0x00001E02, 0x00001E02}},
      Title::Mode::All},
     {"New_3DS i2c",
      SYSTEM_MODULE_UPPER_TITLE_ID,
      {{0x20001E02, 0x20001E02, 0x20001E02, 0x20001E02, 0x20001E02, 0x20001E02}},
      Title::Mode::All},
     {"Safe Mode i2c",
      SYSTEM_MODULE_UPPER_TITLE_ID,
      {{0x00001E03, 0x00001E03, 0x00001E03, 0x00001E03, 0x00001E03, 0x00001E03}},
      Title::Mode::All},
     {"New_3DS Safe Mode i2c",
      SYSTEM_MODULE_UPPER_TITLE_ID,
      {{0x20001E03, 0x20001E03, 0x20001E03, 0x20001E03, 0x20001E03, 0x20001E03}},
      Title::Mode::All},
     {"MCU",
      SYSTEM_MODULE_UPPER_TITLE_ID,
      {{0x00001F02, 0x00001F02, 0x00001F02, 0x00001F02, 0x00001F02, 0x00001F02}},
      Title::Mode::All},
     {"New_3DS MCU",
      SYSTEM_MODULE_UPPER_TITLE_ID,
      {{0x20001F02, 0x20001F02, 0x20001F02, 0x20001F02, 0x20001F02, 0x20001F02}},
      Title::Mode::All},
     {"Safe Mode MCU",
      SYSTEM_MODULE_UPPER_TITLE_ID,
      {{0x00001F03, 0x00001F03, 0x00001F03, 0x00001F03, 0x00001F03, 0x00001F03}},
      Title::Mode::All},
     {"New_3DS Safe Mode MCU",
      SYSTEM_MODULE_UPPER_TITLE_ID,
      {{0x20001F03, 0x20001F03, 0x20001F03, 0x20001F03, 0x20001F03, 0x20001F03}},
      Title::Mode::All},
     {"MIC (Microphone)",
      SYSTEM_MODULE_UPPER_TITLE_ID,
      {{0x00002002, 0x00002002, 0x00002002, 0x00002002, 0x00002002, 0x00002002}},
      Title::Mode::All},
     {"PDN",
      SYSTEM_MODULE_UPPER_TITLE_ID,
      {{0x00002102, 0x00002102, 0x00002102, 0x00002102, 0x00002102, 0x00002102}},
      Title::Mode::All},
     {"Safe Mode PDN",
      SYSTEM_MODULE_UPPER_TITLE_ID,
      {{0x00002103, 0x00002103, 0x00002103, 0x00002103, 0x00002103, 0x00002103}},
      Title::Mode::All},
     {"New_3DS Safe Mode PDN",
      SYSTEM_MODULE_UPPER_TITLE_ID,
      {{0x20002103, 0x20002103, 0x20002103, 0x20002103, 0x20002103, 0x20002103}},
      Title::Mode::All},
     {"PTM (Play time, pedometer, and battery manager)",
      SYSTEM_MODULE_UPPER_TITLE_ID,
      {{0x00002202, 0x00002202, 0x00002202, 0x00002202, 0x00002202, 0x00002202}},
      Title::Mode::All},
     {"New_3DS PTM (Play time, pedometer, and battery manager)",
      SYSTEM_MODULE_UPPER_TITLE_ID,
      {{0x20002202, 0x20002202, 0x20002202, 0x20002202, 0x20002202, 0x20002202}},
      Title::Mode::All},
     {"Safe Mode PTM",
      SYSTEM_MODULE_UPPER_TITLE_ID,
      {{0x00002203, 0x00002203, 0x00002203, 0x00002203, 0x00002203, 0x00002203}},
      Title::Mode::All},
     {"New_3DS Safe Mode PTM",
      SYSTEM_MODULE_UPPER_TITLE_ID,
      {{0x20002203, 0x20002203, 0x20002203, 0x20002203, 0x20002203, 0x20002203}},
      Title::Mode::All},
     {"spi",
      SYSTEM_MODULE_UPPER_TITLE_ID,
      {{0x00002302, 0x00002302, 0x00002302, 0x00002302, 0x00002302, 0x00002302}},
      Title::Mode::All},
     {"New_3DS spi",
      SYSTEM_MODULE_UPPER_TITLE_ID,
      {{0x20002302, 0x20002302, 0x20002302, 0x20002302, 0x20002302, 0x20002302}},
      Title::Mode::All},
     {"Safe Mode spi",
      SYSTEM_MODULE_UPPER_TITLE_ID,
      {{0x00002303, 0x00002303, 0x00002303, 0x00002303, 0x00002303, 0x00002303}},
      Title::Mode::All},
     {"New_3DS Safe Mode spi",
      SYSTEM_MODULE_UPPER_TITLE_ID,
      {{0x20002303, 0x20002303, 0x20002303, 0x20002303, 0x20002303, 0x20002303}},
      Title::Mode::All},
     {"AC (Network manager)",
      SYSTEM_MODULE_UPPER_TITLE_ID,
      {{0x00002402, 0x00002402, 0x00002402, 0x00002402, 0x00002402, 0x00002402}},
      Title::Mode::All},
     {"Safe Mode AC",
      SYSTEM_MODULE_UPPER_TITLE_ID,
      {{0x00002403, 0x00002403, 0x00002403, 0x00002403, 0x00002403, 0x00002403}},
      Title::Mode::All},
     {"New_3DS Safe Mode AC",
      SYSTEM_MODULE_UPPER_TITLE_ID,
      {{0x20002403, 0x20002403, 0x20002403, 0x20002403, 0x20002403, 0x20002403}},
      Title::Mode::All},
     {"Cecd (StreetPass)",
      SYSTEM_MODULE_UPPER_TITLE_ID,
      {{0x00002602, 0x00002602, 0x00002602, 0x00002602, 0x00002602, 0x00002602}},
      Title::Mode::All},
     {"CSND",
      SYSTEM_MODULE_UPPER_TITLE_ID,
      {{0x00002702, 0x00002702, 0x00002702, 0x00002702, 0x00002702, 0x00002702}},
      Title::Mode::All},
     {"Safe Mode CSND",
      SYSTEM_MODULE_UPPER_TITLE_ID,
      {{0x00002703, 0x00002703, 0x00002703, 0x00002703, 0x00002703, 0x00002703}},
      Title::Mode::All},
     {"New_3DS Safe Mode CSND",
      SYSTEM_MODULE_UPPER_TITLE_ID,
      {{0x20002703, 0x20002703, 0x20002703, 0x20002703, 0x20002703, 0x20002703}},
      Title::Mode::All},
     {"DLP (Download Play)",
      SYSTEM_MODULE_UPPER_TITLE_ID,
      {{0x00002802, 0x00002802, 0x00002802, 0x00002802, 0x00002802, 0x00002802}},
      Title::Mode::Recommended},
     {"HTTP",
      SYSTEM_MODULE_UPPER_TITLE_ID,
      {{0x00002902, 0x00002902, 0x00002902, 0x00002902, 0x00002902, 0x00002902}},
      Title::Mode::All},
     {"Safe Mode HTTP",
      SYSTEM_MODULE_UPPER_TITLE_ID,
      {{0x00002903, 0x00002903, 0x00002903, 0x00002903, 0x00002903, 0x00002903}},
      Title::Mode::All},
     {"New_3DS Safe Mode HTTP",
      SYSTEM_MODULE_UPPER_TITLE_ID,
      {{0x20002903, 0x20002903, 0x20002903, 0x20002903, 0x20002903, 0x20002903}},
      Title::Mode::All},
     {"MP",
      SYSTEM_MODULE_UPPER_TITLE_ID,
      {{0x00002A02, 0x00002A02, 0x00002A02, 0x00002A02, 0x00002A02, 0x00002A02}},
      Title::Mode::All},
     {"Safe Mode MP",
      SYSTEM_MODULE_UPPER_TITLE_ID,
      {{0x00002A03, 0x00002A03, 0x00002A03, 0x00002A03, 0x00002A03, 0x00002A03}},
      Title::Mode::All},
     {"NDM",
      SYSTEM_MODULE_UPPER_TITLE_ID,
      {{0x00002B02, 0x00002B02, 0x00002B02, 0x00002B02, 0x00002B02, 0x00002B02}},
      Title::Mode::All},
     {"NIM",
      SYSTEM_MODULE_UPPER_TITLE_ID,
      {{0x00002C02, 0x00002C02, 0x00002C02, 0x00002C02, 0x00002C02, 0x00002C02}},
      Title::Mode::All},
     {"Safe Mode NIM",
      SYSTEM_MODULE_UPPER_TITLE_ID,
      {{0x00002C03, 0x00002C03, 0x00002C03, 0x00002C03, 0x00002C03, 0x00002C03}},
      Title::Mode::All},
     {"New_3DS Safe Mode NIM",
      SYSTEM_MODULE_UPPER_TITLE_ID,
      {{0x20002C03, 0x20002C03, 0x20002C03, 0x20002C03, 0x20002C03, 0x20002C03}},
      Title::Mode::All},
     {"NWM ( Low-level wifi manager )",
      SYSTEM_MODULE_UPPER_TITLE_ID,
      {{0x00002D02, 0x00002D02, 0x00002D02, 0x00002D02, 0x00002D02, 0x00002D02}},
      Title::Mode::All},
     {"Safe Mode NWM",
      SYSTEM_MODULE_UPPER_TITLE_ID,
      {{0x00002D03, 0x00002D03, 0x00002D03, 0x00002D03, 0x00002D03, 0x00002D03}},
      Title::Mode::All},
     {"New_3DS Safe Mode NWM",
      SYSTEM_MODULE_UPPER_TITLE_ID,
      {{0x20002D03, 0x20002D03, 0x20002D03, 0x20002D03, 0x20002D03, 0x20002D03}},
      Title::Mode::All},
     {"Sockets",
      SYSTEM_MODULE_UPPER_TITLE_ID,
      {{0x00002E02, 0x00002E02, 0x00002E02, 0x00002E02, 0x00002E02, 0x00002E02}},
      Title::Mode::All},
     {"Safe Mode Sockets",
      SYSTEM_MODULE_UPPER_TITLE_ID,
      {{0x00002E03, 0x00002E03, 0x00002E03, 0x00002E03, 0x00002E03, 0x00002E03}},
      Title::Mode::All},
     {"New_3DS Safe Mode Sockets",
      SYSTEM_MODULE_UPPER_TITLE_ID,
      {{0x20002E03, 0x20002E03, 0x20002E03, 0x20002E03, 0x20002E03, 0x20002E03}},
      Title::Mode::All},
     {"SSL",
      SYSTEM_MODULE_UPPER_TITLE_ID,
      {{0x00002F02, 0x00002F02, 0x00002F02, 0x00002F02, 0x00002F02, 0x00002F02}},
      Title::Mode::All},
     {"Safe Mode SSL",
      SYSTEM_MODULE_UPPER_TITLE_ID,
      {{0x00002F03, 0x00002F03, 0x00002F03, 0x00002F03, 0x00002F03, 0x00002F03}},
      Title::Mode::All},
     {"New_3DS Safe Mode SSL",
      SYSTEM_MODULE_UPPER_TITLE_ID,
      {{0x20002F03, 0x20002F03, 0x20002F03, 0x20002F03, 0x20002F03, 0x20002F03}},
      Title::Mode::All},
     {"PS ( Process Manager )",
      SYSTEM_MODULE_UPPER_TITLE_ID,
      {{0x00003102, 0x00003102, 0x00003102, 0x00003102, 0x00003102, 0x00003102}},
      Title::Mode::All},
     {"Safe Mode PS",
      SYSTEM_MODULE_UPPER_TITLE_ID,
      {{0x00003103, 0x00003103, 0x00003103, 0x00003103, 0x00003103, 0x00003103}},
      Title::Mode::All},
     {"New_3DS Safe Mode PS",
      SYSTEM_MODULE_UPPER_TITLE_ID,
      {{0x20003103, 0x20003103, 0x20003103, 0x20003103, 0x20003103, 0x20003103}},
      Title::Mode::All},
     {"friends (Friends list)",
      SYSTEM_MODULE_UPPER_TITLE_ID,
      {{0x00003202, 0x00003202, 0x00003202, 0x00003202, 0x00003202, 0x00003202}},
      Title::Mode::All},
     {"Safe Mode friends (Friends list)",
      SYSTEM_MODULE_UPPER_TITLE_ID,
      {{0x00003203, 0x00003203, 0x00003203, 0x00003203, 0x00003203, 0x00003203}},
      Title::Mode::All},
     {"New_3DS Safe Mode friends (Friends list)",
      SYSTEM_MODULE_UPPER_TITLE_ID,
      {{0x20003203, 0x20003203, 0x20003203, 0x20003203, 0x20003203, 0x20003203}},
      Title::Mode::All},
     {"IR (Infrared)",
      SYSTEM_MODULE_UPPER_TITLE_ID,
      {{0x00003302, 0x00003302, 0x00003302, 0x00003302, 0x00003302, 0x00003302}},
      Title::Mode::All},
     {"Safe Mode IR",
      SYSTEM_MODULE_UPPER_TITLE_ID,
      {{0x00003303, 0x00003303, 0x00003303, 0x00003303, 0x00003303, 0x00003303}},
      Title::Mode::All},
     {"New_3DS Safe Mode IR",
      SYSTEM_MODULE_UPPER_TITLE_ID,
      {{0x20003303, 0x20003303, 0x20003303, 0x20003303, 0x20003303, 0x20003303}},
      Title::Mode::All},
     {"BOSS (SpotPass)",
      SYSTEM_MODULE_UPPER_TITLE_ID,
      {{0x00003402, 0x00003402, 0x00003402, 0x00003402, 0x00003402, 0x00003402}},
      Title::Mode::All},
     {"News (Notifications)",
      SYSTEM_MODULE_UPPER_TITLE_ID,
      {{0x00003502, 0x00003502, 0x00003502, 0x00003502, 0x00003502, 0x00003502}},
      Title::Mode::All},
     {"RO",
      SYSTEM_MODULE_UPPER_TITLE_ID,
      {{0x00003702, 0x00003702, 0x00003702, 0x00003702, 0x00003702, 0x00003702}},
      Title::Mode::All},
     {"act",
      SYSTEM_MODULE_UPPER_TITLE_ID,
      {{0x00003802, 0x00003802, 0x00003802, 0x00003802, 0x00003802, 0x00003802}},
      Title::Mode::All},
     {"nfc",
      SYSTEM_MODULE_UPPER_TITLE_ID,
      {{0x00004002, 0x00004002, 0x00004002, 0x00004002, 0x00004002, 0x00004002}},
      Title::Mode::All},
     {"New_3DS mvd",
      SYSTEM_MODULE_UPPER_TITLE_ID,
      {{0x20004102, 0x20004102, 0x20004102, 0x20004102, 0x20004102, 0x20004102}},
      Title::Mode::All},
     {"New_3DS qtm",
      SYSTEM_MODULE_UPPER_TITLE_ID,
      {{0x20004202, 0x20004202, 0x20004202, 0x20004202, 0x20004202, 0x20004202}},
      Title::Mode::All},
     {"NS",
      SYSTEM_MODULE_UPPER_TITLE_ID,
      {{0x00008002, 0x00008002, 0x00008002, 0x00008002, 0x00008002, 0x00008002}},
      Title::Mode::All},
     {"Safe Mode NS",
      SYSTEM_MODULE_UPPER_TITLE_ID,
      {{0x00008003, 0x00008003, 0x00008003, 0x00008003, 0x00008003, 0x00008003}},
      Title::Mode::All},
     {"New_3DS Safe Mode NS",
      SYSTEM_MODULE_UPPER_TITLE_ID,
      {{0x20008003, 0x20008003, 0x20008003, 0x20008003, 0x20008003, 0x20008003}},
      Title::Mode::All}}};

std::vector<u64> BuildFirmwareTitleList(const Title::Mode& mode, u32 region) {
    // Since Australia and Europe share the same title,
    // offset down by one for Australia and above.
    const u32 region_index = region >= 3 ? region - 1 : region;

    const auto titles_with_mode = [mode, region_index](const Title& title) {
        return mode <= title.mode && title.lower_title_id[region_index] != 0;
    };

    std::vector<Title> titles;
    const auto inserter = std::back_inserter(titles);
    std::copy_if(SYSTEM_FIRMWARE.begin(), SYSTEM_FIRMWARE.end(), inserter, titles_with_mode);
    std::copy_if(SYSTEM_APPLICATIONS.begin(), SYSTEM_APPLICATIONS.end(), inserter,
                 titles_with_mode);
    std::copy_if(SYSTEM_DATA_ARCHIVES.begin(), SYSTEM_DATA_ARCHIVES.end(), inserter,
                 titles_with_mode);
    std::copy_if(SYSTEM_APPLETS.begin(), SYSTEM_APPLETS.end(), inserter, titles_with_mode);
    std::copy_if(SHARED_DATA_ARCHIVES.begin(), SHARED_DATA_ARCHIVES.end(), inserter,
                 titles_with_mode);
    std::copy_if(SYSTEM_DATA_ARCHIVES_2.begin(), SYSTEM_DATA_ARCHIVES_2.end(), inserter,
                 titles_with_mode);
    std::copy_if(SYSTEM_MODULES.begin(), SYSTEM_MODULES.end(), inserter, titles_with_mode);

    const auto get_title_id = [region_index](const Title& title) {
        return (static_cast<u64>(title.upper_title_id) << 32) +
               static_cast<u64>(title.lower_title_id[region_index]);
    };

    std::vector<u64> title_ids;
    std::transform(titles.begin(), titles.end(), std::back_inserter(title_ids), get_title_id);
    return title_ids;
}
