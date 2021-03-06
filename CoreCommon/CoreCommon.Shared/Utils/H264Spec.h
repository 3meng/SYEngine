#ifndef __H264_SPEC_H
#define __H264_SPEC_H

#define H264_PROFILE_CONSTRAINED (1 << 9)
#define H264_PROFILE_INTRA (1 << 11)

#define H264_PROFILE_BASELINE 66 //BP
#define H264_PROFILE_CONSTRAINED_BASELINE (H264_PROFILE_BASELINE|H264_PROFILE_CONSTRAINED) //CBP
#define H264_PROFILE_MAIN 77 //MP
#define H264_PROFILE_EXTENDED 88
#define H264_PROFILE_HIGH 100 //HiP

#define H264_PROFILE_HIGH_10 110 //Hi10P
#define H264_PROFILE_HIGH_422 122 //Hi422P
#define H264_PROFILE_HIGH_444 144 //Hi444P
#define H264_PROFILE_HIGH_444_PREDICTIVE 244 //Hi444PP

#define H264_PROFILE_CAVLC_444 44 //Internal
#define H264_PROFILE_HIGH_10_INTRA (H264_PROFILE_HIGH_10|H264_PROFILE_INTRA) //Internal
#define H264_PROFILE_HIGH_422_INTRA (H264_PROFILE_HIGH_422|H264_PROFILE_INTRA) //Internal
#define H264_PROFILE_HIGH_444_INTRA (H264_PROFILE_HIGH_444|H264_PROFILE_INTRA) //Internal

#define NAL_REF_IDC_PRIORITY_HIGHEST 3
#define NAL_REF_IDC_PRIORITY_HIGH 2
#define NAL_REF_IDC_PRIORITY_LOW 1
#define NAL_REF_IDC_PRIORITY_DISPOSABLE 0

#define NAL_UNIT_TYPE_UNSPECIFIED 0
#define NAL_UNIT_TYPE_CODED_SLICE_NON_IDR 1
#define NAL_UNIT_TYPE_CODED_SLICE_DPA 2
#define NAL_UNIT_TYPE_CODED_SLICE_DPB 3
#define NAL_UNIT_TYPE_CODED_SLICE_DPC 4
#define NAL_UNIT_TYPE_CODED_SLICE_IDR 5
#define NAL_UNIT_TYPE_SEI 6
#define NAL_UNIT_TYPE_SPS 7
#define NAL_UNIT_TYPE_PPS 8
#define NAL_UNIT_TYPE_AUD 9
#define NAL_UNIT_TYPE_END_OF_SEQUENCE 10
#define NAL_UNIT_TYPE_END_OF_STREAM 11
#define NAL_UNIT_TYPE_FILLER 12
#define NAL_UNIT_TYPE_SPS_EXT 13

#define SH_SLICE_TYPE_P 0
#define SH_SLICE_TYPE_B 1
#define SH_SLICE_TYPE_I 2 //I Slice
#define SH_SLICE_TYPE_SP 3
#define SH_SLICE_TYPE_SI 4
#define SH_SLICE_TYPE_P_ONLY 5
#define SH_SLICE_TYPE_B_ONLY 6
#define SH_SLICE_TYPE_I_ONLY 7 //I Slice
#define SH_SLICE_TYPE_SP_ONLY 8
#define SH_SLICE_TYPE_SI_ONLY 9

#define H264_INTERLACED_TOP_FIELD 3 //SEI -> pic_struct_present_flag -> pic_struct
#define H264_INTERLACED_BOTTOM_FIELD 6

#define SAR_NONE 0
#define SAR_1_1     1
#define SAR_12_11 2
#define SAR_10_11 3
#define SAR_16_11 4
#define SAR_40_33 5
#define SAR_24_11 6
#define SAR_20_11 7
#define SAR_32_11 8
#define SAR_80_33 9
#define SAR_18_11 10
#define SAR_15_11 11
#define SAR_64_33 12
#define SAR_160_99 13
#define SAR_R_4_3 14
#define SAR_R_3_2 15
#define SAR_R_2_1 16
#define SAR_Extended 255

#define NALU_HEADER_BYTES 1
#define NALU_HEADER_BITS 8

#define H264_ANNEXB_START_CODE_0_BE 0x00000100
#define H264_ANNEXB_START_CODE_1_BE 0x00000001

#define H264_ANNEXB_START_CODE_0_LE 0x00010000
#define H264_ANNEXB_START_CODE_1_LE 0x01000000

#define H264_ANNEXB_3TO4BYTES_AND_BE 0xFFFFFF00
#define H264_ANNEXB_3TO4BYTES_AND_LE 0x00FFFFFF

#if !defined(_WIN32) && defined(_CPU_BE)
#define H264_ANNEXB_START_CODE_0 H264_ANNEXB_START_CODE_0_BE
#define H264_ANNEXB_START_CODE_1 H264_ANNEXB_START_CODE_1_BE
#define H264_ANNEXB_3TO4BYTES_AND H264_ANNEXB_3TO4BYTES_AND_BE
#else
#define H264_ANNEXB_START_CODE_0 H264_ANNEXB_START_CODE_0_LE
#define H264_ANNEXB_START_CODE_1 H264_ANNEXB_START_CODE_1_LE
#define H264_ANNEXB_3TO4BYTES_AND H264_ANNEXB_3TO4BYTES_AND_LE
#endif

#endif //__H264_SPEC_H