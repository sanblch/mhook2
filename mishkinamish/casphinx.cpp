﻿#include "casphinx.h"
#include <Windows.h>
#include <math.h>
#include "MMGlobals.h"

//==============================================================================================================
// Алгоритм подсчёта MFCC изначально был позаимствован из CMU Sphinx, затем
// переписан с использованием Intel IPP От сфинкса остался один только банк
// MEL-фильтров, да и тот деградировал до умножения на табличные значения
//==============================================================================================================

//======================================================================================================================
// MEL-спектр вычисляем по-новому
//======================================================================================================================
#define MM_MEL_TABLE_SIZE 219
typedef struct {
  int index[2];
  float weight[2];
} MM_mel_table;

static MM_mel_table mmt[MM_MEL_TABLE_SIZE] = {
    {0, 0, 0, 0},
    {0, 0, 0, 0},
    {0, 0, 0, 0},
    {0, 0, 0, 0},
    {0, 0, 0, 0},
    {0, 0, 0.0106666666f, 0},
    {0, 0, 0.0213333331f, 0},
    {1, 0, 0.0213333331f, 0},
    {1, 2, 0.0106666666f, 0.00800000038f},
    {2, 0, 0.0160000008f, 0},
    {2, 3, 0.00800000038f, 0.0106666666f},
    {3, 0, 0.0213333331f, 0},
    {4, 0, 0.0213333331f, 0},
    {4, 5, 0.0106666666f, 0.00800000038f},
    {5, 0, 0.0160000008f, 0},
    {5, 6, 0.00800000038f, 0.00639999984f},
    {6, 0, 0.0127999997f, 0},
    {6, 7, 0.00853333343f, 0.00426666671f},
    {6, 7, 0.00426666671f, 0.00853333343f},
    {7, 0, 0.0127999997f, 0},
    {7, 8, 0.00639999984f, 0.00800000038f},
    {8, 0, 0.0160000008f, 0},
    {8, 9, 0.00800000038f, 0.00639999984f},
    {9, 0, 0.0127999997f, 0},
    {9, 10, 0.00853333343f, 0.00426666671f},
    {9, 10, 0.00426666671f, 0.00853333343f},
    {10, 0, 0.0127999997f, 0},
    {10, 11, 0.00639999984f, 0.00639999984f},
    {11, 0, 0.0127999997f, 0},
    {11, 12, 0.00853333343f, 0.0035555556f},
    {11, 12, 0.00426666671f, 0.00711111119f},
    {12, 0, 0.0106666666f, 0},
    {12, 13, 0.00711111119f, 0.0035555556f},
    {12, 13, 0.0035555556f, 0.00711111119f},
    {13, 0, 0.0106666666f, 0},
    {13, 14, 0.00711111119f, 0.00304761901f},
    {13, 14, 0.0035555556f, 0.00609523803f},
    {14, 0, 0.00914285704f, 0},
    {14, 15, 0.00685714278f, 0.00228571426f},
    {14, 15, 0.00457142852f, 0.00457142852f},
    {14, 15, 0.00228571426f, 0.00685714278f},
    {15, 0, 0.00914285704f, 0},
    {15, 16, 0.00609523803f, 0.00304761901f},
    {15, 16, 0.00304761901f, 0.00609523803f},
    {16, 0, 0.00914285704f, 0},
    {16, 17, 0.00685714278f, 0.00200000009f},
    {16, 17, 0.00457142852f, 0.00400000019f},
    {16, 17, 0.00228571426f, 0.00600000005f},
    {17, 0, 0.00800000038f, 0},
    {17, 18, 0.00600000005f, 0.00200000009f},
    {17, 18, 0.00400000019f, 0.00400000019f},
    {17, 18, 0.00200000009f, 0.00600000005f},
    {18, 0, 0.00800000038f, 0},
    {18, 19, 0.00600000005f, 0.00200000009f},
    {18, 19, 0.00400000019f, 0.00400000019f},
    {18, 19, 0.00200000009f, 0.00600000005f},
    {19, 0, 0.00800000038f, 0},
    {19, 20, 0.00600000005f, 0.0017777778f},
    {19, 20, 0.00400000019f, 0.0035555556f},
    {19, 20, 0.00200000009f, 0.00533333328f},
    {20, 0, 0.00711111119f, 0},
    {20, 21, 0.00568888895f, 0.00142222224f},
    {20, 21, 0.00426666671f, 0.00284444448f},
    {20, 21, 0.00284444448f, 0.00426666671f},
    {20, 21, 0.00142222224f, 0.00568888895f},
    {21, 0, 0.00711111119f, 0},
    {21, 22, 0.00533333328f, 0.00159999996f},
    {21, 22, 0.0035555556f, 0.00319999992f},
    {21, 22, 0.0017777778f, 0.00479999976f},
    {22, 0, 0.00639999984f, 0},
    {22, 23, 0.00533333328f, 0.000969696965f},
    {22, 23, 0.00426666671f, 0.00193939393f},
    {22, 23, 0.00319999992f, 0.00290909084f},
    {22, 23, 0.00213333336f, 0.00387878786f},
    {22, 23, 0.00106666668f, 0.00484848442f},
    {23, 0, 0.00581818167f, 0},
    {23, 24, 0.00465454534f, 0.00116363633f},
    {23, 24, 0.00349090924f, 0.00232727267f},
    {23, 24, 0.00232727267f, 0.00349090924f},
    {23, 24, 0.00116363633f, 0.00465454534f},
    {24, 0, 0.00581818167f, 0},
    {24, 25, 0.00484848442f, 0.000969696965f},
    {24, 25, 0.00387878786f, 0.00193939393f},
    {24, 25, 0.00290909084f, 0.00290909084f},
    {24, 25, 0.00193939393f, 0.00387878786f},
    {24, 25, 0.000969696965f, 0.00484848442f},
    {25, 0, 0.00581818167f, 0},
    {25, 26, 0.00465454534f, 0.00106666668f},
    {25, 26, 0.00349090924f, 0.00213333336f},
    {25, 26, 0.00232727267f, 0.00320000015f},
    {25, 26, 0.00116363633f, 0.00426666671f},
    {26, 0, 0.00533333328f, 0},
    {26, 27, 0.00457142852f, 0.000703296741f},
    {26, 27, 0.00380952377f, 0.00140659348f},
    {26, 27, 0.00304761925f, 0.00210989011f},
    {26, 27, 0.00228571426f, 0.00281318696f},
    {26, 27, 0.00152380962f, 0.00351648359f},
    {26, 27, 0.000761904812f, 0.00421978021f},
    {27, 0, 0.00492307683f, 0},
    {27, 28, 0.00410256395f, 0.000820512825f},
    {27, 28, 0.0032820513f, 0.00164102565f},
    {27, 28, 0.00246153842f, 0.00246153842f},
    {27, 28, 0.00164102565f, 0.0032820513f},
    {27, 28, 0.000820512825f, 0.00410256395f},
    {28, 0, 0.00492307683f, 0},
    {28, 29, 0.00421978021f, 0.00060952385f},
    {28, 29, 0.00351648359f, 0.0012190477f},
    {28, 29, 0.00281318696f, 0.00182857143f},
    {28, 29, 0.00210989011f, 0.0024380954f},
    {28, 29, 0.00140659348f, 0.00304761925f},
    {28, 29, 0.000703296741f, 0.00365714286f},
    {29, 0, 0.00426666671f, 0},
    {29, 30, 0.00373333343f, 0.000500000024f},
    {29, 30, 0.00320000015f, 0.00100000005f},
    {29, 30, 0.00266666664f, 0.00150000001f},
    {29, 30, 0.00213333336f, 0.00200000009f},
    {29, 30, 0.00160000008f, 0.00250000018f},
    {29, 30, 0.00106666668f, 0.00300000003f},
    {29, 30, 0.000533333339f, 0.00350000011f},
    {30, 0, 0.00400000019f, 0},
    {30, 31, 0.00350000011f, 0.000500000024f},
    {30, 31, 0.00300000003f, 0.00100000005f},
    {30, 31, 0.00250000018f, 0.00150000001f},
    {30, 31, 0.00200000009f, 0.00200000009f},
    {30, 31, 0.00150000001f, 0.00250000018f},
    {30, 31, 0.00100000005f, 0.00300000003f},
    {30, 31, 0.000500000024f, 0.00350000011f},
    {31, 0, 0.00400000019f, 0},
    {31, 32, 0.00350000011f, 0.000470588246f},
    {31, 32, 0.00300000003f, 0.000941176491f},
    {31, 32, 0.00250000018f, 0.00141176477f},
    {31, 32, 0.00200000009f, 0.00188235298f},
    {31, 32, 0.00150000001f, 0.00235294132f},
    {31, 32, 0.00100000005f, 0.00282352953f},
    {31, 32, 0.000500000024f, 0.00329411775f},
    {32, 0, 0.00376470597f, 0},
    {32, 33, 0.00334640522f, 0.000395061739f},
    {32, 33, 0.00292810472f, 0.000790123479f},
    {32, 33, 0.00250980398f, 0.00118518528f},
    {32, 33, 0.00209150347f, 0.00158024696f},
    {32, 33, 0.00167320261f, 0.00197530887f},
    {32, 33, 0.00125490199f, 0.00237037055f},
    {32, 33, 0.000836601306f, 0.00276543223f},
    {32, 33, 0.000418300653f, 0.00316049391f},
    {33, 0, 0.0035555556f, 0},
    {33, 34, 0.00316049391f, 0.000395061739f},
    {33, 34, 0.00276543223f, 0.000790123479f},
    {33, 34, 0.00237037055f, 0.00118518528f},
    {33, 34, 0.00197530887f, 0.00158024696f},
    {33, 34, 0.00158024696f, 0.00197530887f},
    {33, 34, 0.00118518528f, 0.00237037055f},
    {33, 34, 0.000790123479f, 0.00276543223f},
    {33, 34, 0.000395061739f, 0.00316049391f},
    {34, 0, 0.0035555556f, 0},
    {34, 35, 0.00316049391f, 0.00035555556f},
    {34, 35, 0.00276543223f, 0.000711111119f},
    {34, 35, 0.00237037055f, 0.00106666668f},
    {34, 35, 0.00197530887f, 0.00142222224f},
    {34, 35, 0.00158024696f, 0.0017777778f},
    {34, 35, 0.00118518528f, 0.00213333336f},
    {34, 35, 0.000790123479f, 0.0024888888f},
    {34, 35, 0.000395061739f, 0.00284444448f},
    {35, 0, 0.00319999992f, 0},
    {35, 36, 0.00290909084f, 0.000277056271f},
    {35, 36, 0.00261818175f, 0.000554112543f},
    {35, 36, 0.00232727267f, 0.000831168843f},
    {35, 36, 0.00203636358f, 0.00110822509f},
    {35, 36, 0.0017454545f, 0.00138528144f},
    {35, 36, 0.00145454542f, 0.00166233769f},
    {35, 36, 0.00116363633f, 0.00193939393f},
    {35, 36, 0.000872727251f, 0.00221645017f},
    {35, 36, 0.000581818167f, 0.00249350653f},
    {35, 36, 0.000290909084f, 0.00277056289f},
    {36, 0, 0.00304761901f, 0},
    {36, 37, 0.00274285697f, 0.000290909084f},
    {36, 37, 0.00243809517f, 0.000581818167f},
    {36, 37, 0.00213333336f, 0.000872727309f},
    {36, 37, 0.00182857143f, 0.00116363633f},
    {36, 37, 0.00152380951f, 0.00145454542f},
    {36, 37, 0.00121904758f, 0.00174545462f},
    {36, 37, 0.000914285716f, 0.00203636358f},
    {36, 37, 0.000609523791f, 0.00232727267f},
    {36, 37, 0.000304761896f, 0.00261818175f},
    {37, 0, 0.00290909084f, 0},
    {37, 38, 0.00266666664f, 0.000222222225f},
    {37, 38, 0.00242424221f, 0.000444444449f},
    {37, 38, 0.00218181824f, 0.00066666666f},
    {37, 38, 0.00193939393f, 0.000888888899f},
    {37, 38, 0.00169696962f, 0.00111111102f},
    {37, 38, 0.00145454542f, 0.00133333332f},
    {37, 38, 0.0012121211f, 0.0015555555f},
    {37, 38, 0.000969696965f, 0.0017777778f},
    {37, 38, 0.000727272709f, 0.00199999986f},
    {37, 38, 0.000484848482f, 0.00222222204f},
    {37, 38, 0.000242424241f, 0.00244444446f},
    {38, 0, 0.00266666664f, 0},
    {38, 39, 0.00244444446f, 0.000222222225f},
    {38, 39, 0.00222222204f, 0.000444444449f},
    {38, 39, 0.00199999986f, 0.00066666666f},
    {38, 39, 0.0017777778f, 0.000888888899f},
    {38, 39, 0.0015555555f, 0.00111111102f},
    {38, 39, 0.00133333332f, 0.00133333332f},
    {38, 39, 0.00111111102f, 0.0015555555f},
    {38, 39, 0.000888888899f, 0.0017777778f},
    {38, 39, 0.00066666666f, 0.00199999986f},
    {38, 39, 0.000444444449f, 0.00222222204f},
    {38, 39, 0.000222222225f, 0.00244444446f},
    {39, 0, 0.00266666664f, 0},
    {39, 0, 0.00244444446f, 0},
    {39, 0, 0.00222222204f, 0},
    {39, 0, 0.00199999986f, 0},
    {39, 0, 0.0017777778f, 0},
    {39, 0, 0.0015555555f, 0},
    {39, 0, 0.00133333332f, 0},
    {39, 0, 0.00111111102f, 0},
    {39, 0, 0.000888888899f, 0},
    {39, 0, 0.00066666666f, 0},
    {39, 0, 0.000444444449f, 0},
    {39, 0, 0.000222222225f, 0}};
// 219: { 0, 0, 0, 0 },

//========================================================================
// Вычисляем 40 коэффициентов MEL-спектра по power-спектру
// Мой вариант через предрасчитанную таблицу
//========================================================================
void cas_mel_spec(float *mfspec, float *spec)
// в девичестве - static void fe_mel_spec(fe_t * fe)
{
  int i, k;

  ZeroMemory(mfspec, sizeof(float) * MEL_NUM_FILTERS);

  for (k = 0; k < MM_MEL_TABLE_SIZE; k++) {
    for (i = 0; i < 2; i++) {
      mfspec[mmt[k].index[i]] += mmt[k].weight[i] * spec[k];
    }
  }
}
