/*******************\
* RDS COUNTRY CODES *
\*******************/

/*
 * These codes are used when setting up a
 * Programme Identifier. The format of the data
 * is <country code>(bits 8-4)<extended country code>
 * (bits 4-1). The values here come from the RDS standard.
 */

/* European Broadcasting Area */
#define RDS_COUNTRY_AL		0x09E0	/* Albania */
#define RDS_COUNTRY_DZ		0x02E0	/* Algeria */
#define RDS_COUNTRY_AD		0x03E0	/* Andorra */
#define RDS_COUNTRY_AM		0x0AE4	/* Armenia */
#define RDS_COUNTRY_AT		0x0AE0	/* Austria */
#define RDS_COUNTRY_AZ		0x0BE3	/* Azerbaijan */
#define RDS_COUNTRY_PT		0x08E4	/* Azores (Portugal) */
#define RDS_COUNTRY_BE		0x06E0	/* Belgium */
#define RDS_COUNTRY_BY		0x0FE3	/* Belarus */
#define RDS_COUNTRY_BA		0x0FE4	/* Bosnia Herzegovina */
#define RDS_COUNTRY_BG		0x08E1	/* Bulgaria */
#define RDS_COUNTRY_ES		0x0EE2	/* Canaries (Spain) */
#define RDS_COUNTRY_HR		0x0CE3	/* Croatia */
#define RDS_COUNTRY_CY		0x02E1	/* Cyprus */
#define RDS_COUNTRY_CZ		0x02E2	/* Czech Republic */
#define RDS_COUNTRY_DK		0x09E1	/* Denmark */
#define RDS_COUNTRY_EG		0x0FE0	/* Egypt */
#define RDS_COUNTRY_EE		0x02E4	/* Estonia */
#define RDS_COUNTRY_DK		0x09E1	/* Faroe (Denmark) */
#define RDS_COUNTRY_FI		0x06E1	/* Finland */
#define RDS_COUNTRY_FR		0x0FE1	/* France */
#define RDS_COUNTRY_GE		0x0CE4	/* Georgia */
#define RDS_COUNTRY_DE_1	0x0DE0	/* Germany (East) */
#define RDS_COUNTRY_DE_2	0x01E0	/* Germany (West) */
#define RDS_COUNTRY_GI		0x0AE1	/* Gibraltar (United Kingdom) */
#define RDS_COUNTRY_GR		0x01E1	/* Greece */
#define RDS_COUNTRY_HU		0x0BE0	/* Hungary */
#define RDS_COUNTRY_IS		0x0AE2	/* Iceland */
#define RDS_COUNTRY_IQ		0x0BE1	/* Iraq */
#define RDS_COUNTRY_IE		0x02E3	/* Ireland */
#define RDS_COUNTRY_IL		0x04E0	/* Israel */
#define RDS_COUNTRY_IT		0x05E0	/* Italy */
#define RDS_COUNTRY_JO		0x05E1	/* Jordan */
#define RDS_COUNTRY_LV		0x09E3	/* Latvia */
#define RDS_COUNTRY_LB		0x0AE3	/* Lebanon */
#define RDS_COUNTRY_LY		0x0DE1	/* Libya */
#define RDS_COUNTRY_LI		0x09E2	/* Liechtenstein */
#define RDS_COUNTRY_LT		0x0CE2	/* Lithuania */
#define RDS_COUNTRY_LU		0x07E1	/* Luxembourg */
#define RDS_COUNTRY_MK		0x04E3	/* Macedonia */
#define RDS_COUNTRY_PT		0x08E4	/* Madeira (Portugal) */
#define RDS_COUNTRY_MT		0x0CE0	/* Malta */
#define RDS_COUNTRY_MD		0x01E4	/* Moldova */
#define RDS_COUNTRY_MC		0x0BE2	/* Monaco */
#define RDS_COUNTRY_ME		0x01E3	/* Montenegro */
#define RDS_COUNTRY_MA		0x01E2	/* Morocco */
#define RDS_COUNTRY_NL		0x08E3	/* Netherlands */
#define RDS_COUNTRY_NO		0x0FE2	/* Norway */
#define RDS_COUNTRY_PS		0x08E0	/* Palestine */
#define RDS_COUNTRY_PL		0x03E2	/* Poland */
#define RDS_COUNTRY_PT		0x08E4	/* Portugal */
#define RDS_COUNTRY_RO		0x0EE1	/* Romania */
#define RDS_COUNTRY_RU		0x07E0	/* Russian Federation */
#define RDS_COUNTRY_SM		0x03E1	/* San Marino */
#define RDS_COUNTRY_RS		0x0DE2	/* Serbia */
#define RDS_COUNTRY_SK		0x05E2	/* Slovakia */
#define RDS_COUNTRY_SI		0x09E4	/* Slovenia */
#define RDS_COUNTRY_ES		0x0EE2	/* Spain */
#define RDS_COUNTRY_SE		0x0EE3	/* Sweden */
#define RDS_COUNTRY_CH		0x04E1	/* Switzerland */
#define RDS_COUNTRY_SY		0x06E2	/* Syrian Arab Republic */
#define RDS_COUNTRY_TN		0x07E2	/* Tunisia */
#define RDS_COUNTRY_TR		0x03E3	/* Turkey */
#define RDS_COUNTRY_UA		0x06E4	/* Ukraine */
#define RDS_COUNTRY_GB		0x0CE1	/* United Kingdom */
#define RDS_COUNTRY_VA		0x04E2	/* Vatican City State */
/* African Broadcasting Area */
#define RDS_COUNTRY_AO		0x06D0	/* Angola */
#define RDS_COUNTRY_BI		0x09D1	/* Burundi */
#define RDS_COUNTRY_BJ		0x0ED0	/* Benin */
#define RDS_COUNTRY_BF		0x0BD0	/* Burkina Faso */
#define RDS_COUNTRY_BW		0x0BD1	/* Botswana */
#define RDS_COUNTRY_CM		0x01D0	/* Cameroon */
#define RDS_COUNTRY_CV		0x06D1	/* Cape Verde */
#define RDS_COUNTRY_CF		0x02D0	/* Central African Republic */
#define RDS_COUNTRY_TD		0x09D2	/* Chad */
#define RDS_COUNTRY_KM		0x0CD1	/* Comoros */
#define RDS_COUNTRY_CD		0x0BD2	/* Democratic Republic of Congo */
#define RDS_COUNTRY_CG		0x0CD0	/* Congo */
#define RDS_COUNTRY_CI		0x0CD2	/* Cote d'Ivoire */
#define RDS_COUNTRY_DJ		0x03D0	/* Djibouti */
#define RDS_COUNTRY_GQ		0x07D0	/* Equatorial Guinea */
#define RDS_COUNTRY_ER		0x0FD2	/* Eritrea */
#define RDS_COUNTRY_ET		0x0ED1	/* Ethiopia */
#define RDS_COUNTRY_GA		0x08D0	/* Gabon */
#define RDS_COUNTRY_GM		0x08D1	/* Gambia */
#define RDS_COUNTRY_GH		0x03D1	/* Ghana */
#define RDS_COUNTRY_GW		0x0AD2	/* Guinea-Bissau */
#define RDS_COUNTRY_GQ		0x07D0	/* Equatorial Guinea */
#define RDS_COUNTRY_GN		0x09D0	/* Republic of Guinea */
#define RDS_COUNTRY_KE		0x06D2	/* Kenya */
#define RDS_COUNTRY_LR		0x02D1	/* Liberia */
#define RDS_COUNTRY_LS		0x06D3	/* Lesotho */
#define RDS_COUNTRY_MU		0x0AD3	/* Mauritius */
#define RDS_COUNTRY_MG		0x04D0	/* Madagascar */
#define RDS_COUNTRY_MW		0x0FD0	/* Malawi */
#define RDS_COUNTRY_ML		0x05D0	/* Mali */
#define RDS_COUNTRY_MR		0x04D1	/* Mauritania */
#define RDS_COUNTRY_MZ		0x03D2	/* Mozambique */
#define RDS_COUNTRY_NA		0x01D1	/* Namibia */
#define RDS_COUNTRY_NE		0x08D2	/* Niger */
#define RDS_COUNTRY_NG		0x0FD1	/* Nigeria */
#define RDS_COUNTRY_RW		0x05D3	/* Rwanda */
#define RDS_COUNTRY_ST		0x05D1	/* Sao Tome & Principe */
#define RDS_COUNTRY_SC		0x08D3	/* Seychelles */
#define RDS_COUNTRY_SN		0x07D1	/* Senegal */
#define RDS_COUNTRY_SL		0x01D2	/* Sierra Leone */
#define RDS_COUNTRY_SO		0x07D2	/* Somalia */
#define RDS_COUNTRY_ZA		0x0AD0	/* South Africa */
#define RDS_COUNTRY_SD		0x0CD3	/* Sudan */
#define RDS_COUNTRY_SZ		0x05D2	/* Swaziland */
#define RDS_COUNTRY_TG		0x0DD0	/* Togo */
#define RDS_COUNTRY_TZ		0x0DD1	/* Tanzania */
#define RDS_COUNTRY_UG		0x04D2	/* Uganda */
#define RDS_COUNTRY_EH		0x03D3	/* Western Sahara */
#define RDS_COUNTRY_ZM		0x0ED2	/* Zambia */
#define RDS_COUNTRY_ZW		0x02D2	/* Zimbabwe */
/* ITU Region 2 */
#define RDS_COUNTRY_AI		0x01A2	/* Anguilla */
#define RDS_COUNTRY_AG		0x02A2	/* Antigua and Barbuda */
#define RDS_COUNTRY_AR		0x0AA2	/* Argentina */
#define RDS_COUNTRY_AW		0x03A4	/* Aruba */
#define RDS_COUNTRY_BS		0x0FA2	/* Bahamas */
#define RDS_COUNTRY_BB		0x05A2	/* Barbados */
#define RDS_COUNTRY_BZ		0x06A2	/* Belize */
#define RDS_COUNTRY_BM		0x0CA2	/* Bermuda */
#define RDS_COUNTRY_BO		0x01A3	/* Bolivia */
#define RDS_COUNTRY_BR		0x0BA2	/* Brazil */
#define RDS_COUNTRY_CA_B	0x0BA1	/* Canada */
#define RDS_COUNTRY_CA_C	0x0CA1
#define RDS_COUNTRY_CA_D	0x0DA1
#define RDS_COUNTRY_CA_E	0x0EA1
#define RDS_COUNTRY_KY		0x07A2	/* Cayman Islands */
#define RDS_COUNTRY_CL		0x0CA3	/* Chile */
#define RDS_COUNTRY_CO		0x02A3	/* Colombia */
#define RDS_COUNTRY_CR		0x08A2	/* Costa Rica */
#define RDS_COUNTRY_CU		0x09A2	/* Cuba */
#define RDS_COUNTRY_DM		0x0AA3	/* Dominica */
#define RDS_COUNTRY_DO		0x0BA3	/* Dominican Republic */
#define RDS_COUNTRY_EC		0x03A2	/* Ecuador */
#define RDS_COUNTRY_SV		0x0CA4	/* El Salvador */
#define RDS_COUNTRY_FK		0x04A2	/* Falkland Islands */
#define RDS_COUNTRY_GF		0x05A3	/* French Guiana */
#define RDS_COUNTRY_GL		0x0FA1	/* Greenland */
#define RDS_COUNTRY_GD		0x0DA3	/* Grenada */
#define RDS_COUNTRY_GP		0x0EA2	/* Guadeloupe */
#define RDS_COUNTRY_GT		0x01A4	/* Guatemala */
#define RDS_COUNTRY_GY		0x0FA3	/* Guyana */
#define RDS_COUNTRY_HT		0x0DA4	/* Haiti */
#define RDS_COUNTRY_HN		0x02A4	/* Honduras */
#define RDS_COUNTRY_JM		0x03A3	/* Jamaica */
#define RDS_COUNTRY_MQ		0x04A3	/* Martinique */
#define RDS_COUNTRY_MX_B	0x0BA5	/* Mexico */
#define RDS_COUNTRY_MX_D	0x0DA5
#define RDS_COUNTRY_MX_E	0x0EA5
#define RDS_COUNTRY_MX_F	0x0FA5
#define RDS_COUNTRY_MS		0x05A4	/* Montserrat */
#define RDS_COUNTRY_AN		0x0DA2	/* Netherlands Antilles */
#define RDS_COUNTRY_NI		0x07A3	/* Nicaragua */
#define RDS_COUNTRY_PA		0x09A3	/* Panama */
#define RDS_COUNTRY_PY		0x06A3	/* Paraguay */
#define RDS_COUNTRY_PE		0x07A4	/* Peru */
#define RDS_COUNTRY_PR_1	0x01A0	/* Puerto Rico */
#define RDS_COUNTRY_PR_2	0x02A0
#define RDS_COUNTRY_PR_3	0x03A0
#define RDS_COUNTRY_PR_4	0x04A0
#define RDS_COUNTRY_PR_5	0x05A0
#define RDS_COUNTRY_PR_6	0x06A0
#define RDS_COUNTRY_PR_7	0x07A0
#define RDS_COUNTRY_PR_8	0x08A0
#define RDS_COUNTRY_PR_9	0x09A0
#define RDS_COUNTRY_PR_A	0x0AA0
#define RDS_COUNTRY_PR_B	0x0BA0
#define RDS_COUNTRY_PR_D	0x0DA0
#define RDS_COUNTRY_PR_E	0x0EA0
#define RDS_COUNTRY_KN		0x0AA4	/* Saint Kitts */
#define RDS_COUNTRY_LC		0x0BA4	/* Saint Lucia */
#define RDS_COUNTRY_PM		0x0FA6	/* St Pierre and Miquelon */
#define RDS_COUNTRY_VC		0x0CA5	/* Saint Vincent */
#define RDS_COUNTRY_SR		0x08A4	/* Suriname */
#define RDS_COUNTRY_TT		0x06A4	/* Trinidad and Tobago */
#define RDS_COUNTRY_TC		0x0EA3	/* Turks and Caicos Islands */
#define RDS_COUNTRY_US_1	0x01A0	/* United States of America */
#define RDS_COUNTRY_US_2	0x02A0
#define RDS_COUNTRY_US_3	0x03A0
#define RDS_COUNTRY_US_4	0x04A0
#define RDS_COUNTRY_US_5	0x05A0
#define RDS_COUNTRY_US_6	0x06A0
#define RDS_COUNTRY_US_7	0x07A0
#define RDS_COUNTRY_US_8	0x08A0
#define RDS_COUNTRY_US_9	0x09A0
#define RDS_COUNTRY_US_A	0x0AA0
#define RDS_COUNTRY_US_B	0x0BA0
#define RDS_COUNTRY_US_D	0x0DA0
#define RDS_COUNTRY_US_E	0x0EA0
#define RDS_COUNTRY_UY		0x09A4	/* Uruguay */
#define RDS_COUNTRY_VE		0x0EA4	/* Venezuela */
#define RDS_COUNTRY_VG		0x0FA5	/* Virgin Islands [British] */
#define RDS_COUNTRY_VI_1	0x01A0	/* Virgin Islands [USA] */
#define RDS_COUNTRY_VI_2	0x02A0
#define RDS_COUNTRY_VI_3	0x03A0
#define RDS_COUNTRY_VI_4	0x04A0
#define RDS_COUNTRY_VI_5	0x05A0
#define RDS_COUNTRY_VI_6	0x06A0
#define RDS_COUNTRY_VI_7	0x07A0
#define RDS_COUNTRY_VI_8	0x08A0
#define RDS_COUNTRY_VI_9	0x09A0
#define RDS_COUNTRY_VI_A	0x0AA0
#define RDS_COUNTRY_VI_B	0x0BA0
#define RDS_COUNTRY_VI_D	0x0DA0
#define RDS_COUNTRY_VI_E	0x0EA0
#define RDS_COUNTRY_AF		0x0AF0	/* Afghanistan */
/* ITU Region 3*/
#define RDS_COUNTRY_AU_1	0x01F0	/* Australia Capital Territory */
#define RDS_COUNTRY_AU_2	0x02F0	/* New South Wales */
#define RDS_COUNTRY_AU_3	0x03F0	/* Victoria */
#define RDS_COUNTRY_AU_4	0x04F0	/* Queensland */
#define RDS_COUNTRY_AU_5	0x05F0	/* South Australia */
#define RDS_COUNTRY_AU_6	0x06F0	/* Western Australia */
#define RDS_COUNTRY_AU_7	0x07F0	/* Tasmania */
#define RDS_COUNTRY_AU_8	0x08F0	/* Northern Territory */
#define RDS_COUNTRY_BD		0x03F1	/* Bangladesh */
#define RDS_COUNTRY_BH		0x0EF0	/* Bahrain */
#define RDS_COUNTRY_BN		0x0BF1	/* Brunei Darussalam */
#define RDS_COUNTRY_BT		0x02F1	/* Bhutan */
#define RDS_COUNTRY_KH		0x03F2	/* Cambodia */
#define RDS_COUNTRY_CN		0x0CF0	/* China */
#define RDS_COUNTRY_FJ		0x05F1	/* Fiji */
#define RDS_COUNTRY_HK		0x0FF1	/* Hong Kong */
#define RDS_COUNTRY_IN		0x05F2	/* India */
#define RDS_COUNTRY_ID		0x0CF2	/* Indonesia */
#define RDS_COUNTRY_IR		0x08F1	/* Iran */
#define RDS_COUNTRY_JP		0x09F2	/* Japan */
#define RDS_COUNTRY_KZ		0x0DE3	/* Kazakhstan */
#define RDS_COUNTRY_KI		0x01F1	/* Kiribati */
#define RDS_COUNTRY_KR		0x0EF1	/* Korea [South] */
#define RDS_COUNTRY_KP		0x0DF0	/* Korea [North] */
#define RDS_COUNTRY_KW		0x01F2	/* Kuwait */
#define RDS_COUNTRY_KG		0x03E4	/* Kyrghyzstan */
#define RDS_COUNTRY_LA		0x01F3	/* Laos */
#define RDS_COUNTRY_MO		0x06F2	/* Macao */
#define RDS_COUNTRY_MY		0x0FF0	/* Malaysia */
#define RDS_COUNTRY_MV		0x0BF2	/* Maldives */
#define RDS_COUNTRY_FM		0x0EF3	/* Micronesia */
#define RDS_COUNTRY_MN		0x0FF3	/* Mongolia */
#define RDS_COUNTRY_MM		0x0BF0	/* Myanmar [Burma] */
#define RDS_COUNTRY_NP		0x0EF2	/* Nepal */
#define RDS_COUNTRY_NR		0x07F1	/* Nauru */
#define RDS_COUNTRY_NZ		0x09F1	/* New Zealand */
#define RDS_COUNTRY_OM		0x06F1	/* Oman */
#define RDS_COUNTRY_PK		0x04F1	/* Pakistan */
#define RDS_COUNTRY_PH		0x08F2	/* Philippines */
#define RDS_COUNTRY_PG		0x09F3	/* Papua New Guinea */
#define RDS_COUNTRY_QA		0x02F2	/* Qatar */
#define RDS_COUNTRY_SA		0x09F0	/* Saudi Arabia */
#define RDS_COUNTRY_SB		0x0AF1	/* Soloman Islands */
#define RDS_COUNTRY_WS		0x04F2	/* Samoa */
#define RDS_COUNTRY_SG		0x0AF2	/* Singapore */
#define RDS_COUNTRY_LK		0x0CF1	/* Sri Lanka */
#define RDS_COUNTRY_TW		0x0DF1	/* Taiwan */
#define RDS_COUNTRY_TJ		0x05E3	/* Tajikistan */
#define RDS_COUNTRY_TH		0x02F3	/* Thailand */
#define RDS_COUNTRY_TO		0x03F3	/* Tonga */
#define RDS_COUNTRY_TM		0x0EE4	/* Turkmenistan */
#define RDS_COUNTRY_AE		0x0DF2	/* UAE */
#define RDS_COUNTRY_UZ		0x0BE4	/* Uzbekistan */
#define RDS_COUNTRY_VN		0x07F2	/* Vietnam */
#define RDS_COUNTRY_VU		0x0FF2	/* Vanuatu */
#define RDS_COUNTRY_YE		0x0BF3	/* Yemen */
