/*
 * Copyright (c) 2021 Arm Limited. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include "UseCaseHandler.hpp"

#include "Classifier.hpp"
#include "InputFiles.hpp"
#include "Yolov8npose.hpp"
#include "UseCaseCommonUtils.hpp"
#include "hal.h"

#include <inttypes.h>
/////////////////////////
#include "DetectionResult.hpp"
#include "ImageUtils.hpp"
#include <forward_list>
#include <algorithm>
#include <math.h>
#include <stdint.h>
#define MAX_TRACKED_ALGO_RES  10
#define COLOR_DEPTH	1 // 8bit per pixel FU
typedef enum
{
	MONO_FRAME=0,
	RAWBAYER_FRAME,
	YUV_FRAME
}enum_frameFormat;


typedef struct
{
    uint32_t x;
    uint32_t y;
    uint32_t width;
    uint32_t height;
}struct__box;

typedef struct
{
	struct__box bbox;
    uint32_t time_of_existence;
    uint32_t is_reliable;
}struct_MotionTarget;



typedef struct
{
	struct__box upper_body_bbox;
    uint32_t upper_body_scale;
    uint32_t upper_body_score;
    uint32_t upper_body_num_frames_since_last_redetection_attempt;
    struct__box head_bbox;
    uint32_t head_scale;
    uint32_t head_score;
    uint32_t head_num_frames_since_last_redetection_attempt;
    uint32_t octave;
    uint32_t time_of_existence;
    uint32_t isVerified;
}struct_Human;

typedef struct
{
    int num_hot_pixels ;
    struct_MotionTarget Emt[MAX_TRACKED_ALGO_RES]; ; //ecv::motion::Target* *tracked_moving_targets;
    int frame_count ;
    short num_tracked_moving_targets;
    short num_tracked_human_targets ;
    bool humanPresence ;
    struct_Human ht[MAX_TRACKED_ALGO_RES];  //TrackedHumanTarget* *tracked_human_targets;
    int num_reliable_moving_targets;
    int verifiedHumansExist;
}struct_algoResult;

typedef struct boxabs {
    float left, right, top, bot;
} boxabs;


typedef struct branch {
    int resolution;
    int num_box;
    float *anchor;
    int8_t *tf_output;
    float scale;
    int zero_point;
    size_t size;
    float scale_x_y;
} branch;

typedef struct network {
    int input_w;
    int input_h;
    int num_classes;
    int num_branch;
    branch *branchs;
    int topN;
} network;


typedef struct box {
    float x, y, w, h;
} box;

typedef struct detection{
    box bbox;
    float *prob;
    float objectness;
} detection;

struct_algoResult algoresult;



typedef struct detection_yolov8{
    box bbox;
    float confidence;
    float index;

} detection_yolov8;


typedef struct struct_human_pose{
    uint32_t x;
    uint32_t y;
    float score;
} struct_human_pose;

#define HUMAN_POSE_POINT_NUM 17

typedef struct struct_human_pose_17{
    struct_human_pose hpr[HUMAN_POSE_POINT_NUM];
} struct_human_pose_17;

typedef struct detection_yolov8_pose{
    box bbox;
    float confidence;
    float index;
    struct_human_pose hpr[HUMAN_POSE_POINT_NUM];
} detection_yolov8_pose;

#define INPUT_IMAGE_SIZE 192

static void softmax(float *input, size_t input_len) {
  assert(input);
  // assert(input_len >= 0);  Not needed

  float m = -INFINITY;
  for (size_t i = 0; i < input_len; i++) {
    if (input[i] > m) {
      m = input[i];
    }
  }

  float sum = 0.0;
  for (size_t i = 0; i < input_len; i++) {
    sum += expf(input[i] - m);
  }

  float offset = m + logf(sum);
  for (size_t i = 0; i < input_len; i++) {
    input[i] = expf(input[i] - offset);
  }
}

std::string coco_classes[] = {"person","bicycle","car","motorcycle","airplane","bus","train","truck","boat","traffic light","fire hydrant","stop sign","parking meter","bench","bird","cat","dog","horse","sheep","cow","elephant","bear","zebra","giraffe","backpack","umbrella","handbag","tie","suitcase","frisbee","skis","snowboard","sports ball","kite","baseball bat","baseball glove","skateboard","surfboard","tennis racket","bottle","wine glass","cup","fork","knife","spoon","bowl","banana","apple","sandwich","orange","broccoli","carrot","hot dog","pizza","donut","cake","chair","couch","potted plant","bed","dining table","toilet","tv","laptop","mouse","remote","keyboard","cell phone","microwave","oven","toaster","sink","refrigerator","book","clock","vase","scissors","teddy bear","hair drier","toothbrush"};
int coco_ids[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 27, 28, 31,
                      32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56,
                      57, 58, 59, 60, 61, 62, 63, 64, 65, 67, 70, 72, 73, 74, 75, 76, 77, 78, 79, 80, 81, 82, 84, 85,
                      86, 87, 88, 89, 90};

#define RGB888_TO_RGB565(R8,G8,B8)  ((((R8>>3) & 0x1F) << 11) |     \
                                     (((G8>>2) & 0x3F) << 5)  |     \
                                     ((B8>>3) & 0x1F))

uint16_t COLOR_RED  = RGB888_TO_RGB565(  255, 0,  0);


std::pair<uint8_t, uint8_t> human_pose_upper_body_adjacent_demo[]=
{
    std::make_pair(0, 1),//nose to left eye
    std::make_pair(0, 2),//nose to right eye
    std::make_pair(1, 3),//left eye to left ear
    std::make_pair(2, 4),//right eye to right ear


    std::make_pair(3, 5),//left eye to left shoulder
    std::make_pair(4, 6),//right eye to right shoulder


    std::make_pair(5, 6),//left shoulder to right shoulder




    std::make_pair(5, 11),//left shoulder to left thigh

    std::make_pair(6, 12),//right shoulder to right thigh

    std::make_pair(11, 12),//left thigh to right thigh

};

std::pair<uint8_t, uint8_t> human_pose_left_lower_body_adjacent_demo[]=
{
    std::make_pair(5, 7),//left shoulder to left elbow
    std::make_pair(7, 9),//left elbow to left hand
    std::make_pair(11, 13),//left thigh to left knee
    std::make_pair(13, 15),//left knee to left foot
};

std::pair<uint8_t, uint8_t> human_pose_right_lower_body_adjacent_demo[]=
{
    std::make_pair(6, 8),//right shoulder to right elbow
    std::make_pair(8, 10),//right elbow to right hand
    std::make_pair(12, 14),//right thigh to right knee
    std::make_pair(14, 16),//right knee to right foot
};
//////////////////////



using ImgClassClassifier = arm::app::Classifier;

namespace arm {
namespace app {


/////////////////////////////////////
    static int sort_class;

void free_dets(std::forward_list<detection> &dets){
    std::forward_list<detection>::iterator it;
    for ( it = dets.begin(); it != dets.end(); ++it ){
        free(it->prob);
    }
}

float sigmoid(float x)
{
    return 1.f/(1.f + exp(-x));
}

bool det_objectness_comparator(detection &pa, detection &pb)
{
    return pa.objectness < pb.objectness;
}

void insert_topN_det(std::forward_list<detection> &dets, detection det){
    std::forward_list<detection>::iterator it;
    std::forward_list<detection>::iterator last_it;
    for ( it = dets.begin(); it != dets.end(); ++it ){
        if(it->objectness > det.objectness)
            break;
        last_it = it;
    }
    if(it != dets.begin()){
        dets.emplace_after(last_it, det);
        free(dets.begin()->prob);
        dets.pop_front();
    }
    else{
        free(det.prob);
    }
}

// NMS part

float overlap(float x1, float w1, float x2, float w2)
{
    float l1 = x1 - w1/2;
    float l2 = x2 - w2/2;
    float left = l1 > l2 ? l1 : l2;
    float r1 = x1 + w1/2;
    float r2 = x2 + w2/2;
    float right = r1 < r2 ? r1 : r2;
    return right - left;
}

float box_intersection(box a, box b)
{
    float w = overlap(a.x, a.w, b.x, b.w);
    float h = overlap(a.y, a.h, b.y, b.h);
    if(w < 0 || h < 0) return 0;
    float area = w*h;
    return area;
}

float box_union(box a, box b)
{
    float i = box_intersection(a, b);
    float u = a.w*a.h + b.w*b.h - i;
    return u;
}

float box_iou(box a, box b)
{
    float I = box_intersection(a, b);
    float U = box_union(a, b);
    if (I == 0 || U == 0) {
        return 0;
    }
    return I / U;
}

bool det_comparator(detection &pa, detection &pb)
{
    return pa.prob[sort_class] > pb.prob[sort_class];
}

void do_nms_sort(std::forward_list<detection> &dets, int classes, float thresh)
{
    int k;

    for (k = 0; k < classes; ++k) {
        sort_class = k;
        dets.sort(det_comparator);

        for (std::forward_list<detection>::iterator it=dets.begin(); it != dets.end(); ++it){
            if (it->prob[k] == 0) continue;
            for (std::forward_list<detection>::iterator itc=std::next(it, 1); itc != dets.end(); ++itc){
                if (itc->prob[k] == 0) continue;
                if (box_iou(it->bbox, itc->bbox) > thresh) {
                    itc->prob[k] = 0;
                }
            }
        }
    }
}


boxabs box_c(box a, box b) {
    boxabs ba;//
    ba.top = 0;
    ba.bot = 0;
    ba.left = 0;
    ba.right = 0;
    ba.top = fmin(a.y - a.h / 2, b.y - b.h / 2);
    ba.bot = fmax(a.y + a.h / 2, b.y + b.h / 2);
    ba.left = fmin(a.x - a.w / 2, b.x - b.w / 2);
    ba.right = fmax(a.x + a.w / 2, b.x + b.w / 2);
    return ba;
}


float box_diou(box a, box b)
{
    boxabs ba = box_c(a, b);
    float w = ba.right - ba.left;
    float h = ba.bot - ba.top;
    float c = w * w + h * h;
    float iou = box_iou(a, b);
    if (c == 0) {
        return iou;
    }
    float d = (a.x - b.x) * (a.x - b.x) + (a.y - b.y) * (a.y - b.y);
    float u = pow(d / c, 0.6);
    float diou_term = u;

    return iou - diou_term;
}


void diounms_sort(std::forward_list<detection> &dets, int classes, float thresh)
{
    int k;

    for (k = 0; k < classes; ++k) {
        sort_class = k;
        dets.sort(det_comparator);

        for (std::forward_list<detection>::iterator it=dets.begin(); it != dets.end(); ++it){
            if (it->prob[k] == 0) continue;
            for (std::forward_list<detection>::iterator itc=std::next(it, 1); itc != dets.end(); ++itc){
                if (itc->prob[k] == 0) continue;
                if (box_diou(it->bbox, itc->bbox) > thresh) {
                    itc->prob[k] = 0;
                }
            }
        }
    }
}


bool yolov8_det_comparator(detection_yolov8 &pa, detection_yolov8 &pb)
{
    return pa.confidence > pb.confidence;
}

/***
 * caculate yolov8 pose bbox dequant value
 * int dims_cnt_1: the index for dim 1 (max index = 756)
 * int dims_cnt_2: the index for dim 2 (max index = 64 or 1)
 * TfLiteTensor* output: pointer for output tensor
 * **/
float yolov8_pose_bbox_dequant_value(int dims_cnt_1, int dims_cnt_2,TfLiteTensor* output)
{
	int value =  output->data.int8[ dims_cnt_2 + dims_cnt_1 * output->dims->data[2]];

	float deq_value = ((float) value-(float)((TfLiteAffineQuantization*)(output->quantization.params))->zero_point->data[0]) * ((TfLiteAffineQuantization*)(output->quantization.params))->scale->data[0];
	return deq_value;
}

/***
 * caculate yolov8 pose key points dequant value
 * int dims_cnt_1: the index for dim 1 (max index = 756)
 * int dims_cnt_2: the index for dim 2 (max index = 51)
 * TfLiteTensor* output: pointer for output tensor
 * float anchor_val_0: anchor value for dim 1
 * float anchor_val_1: anchor value for dim 2
 * float anchor_val_1: stride value
 * **/
float yolov8_pose_key_pts_dequant_value(int dims_cnt_1, int dims_cnt_2,TfLiteTensor* output, float anchor_val_0, float anchor_val_1 , float stride_val)
{
	int value =  output->data.int8[ dims_cnt_2 + dims_cnt_1 * output->dims->data[2]];

	float deq_value = ((float) value-(float)((TfLiteAffineQuantization*)(output->quantization.params))->zero_point->data[0]) * ((TfLiteAffineQuantization*)(output->quantization.params))->scale->data[0];

	if(dims_cnt_2 % 3==0)
	{
		deq_value = ( deq_value * 2.0 +  (anchor_val_0 - 0.5) )* stride_val;
	}
	else if(dims_cnt_2 % 3==1)
	{
		deq_value = ( deq_value * 2.0 +  (anchor_val_1 - 0.5) )* stride_val;
	}
	else
	{
		deq_value = sigmoid(deq_value);
	}
	return deq_value;
}
void  yolov8_NMSBoxes(std::vector<box> &boxes,std::vector<float> &confidences,float modelScoreThreshold,float modelNMSThreshold,std::vector<int>& nms_result)
{
    detection_yolov8 yolov8_bbox;
    std::vector<detection_yolov8> yolov8_bboxes{};
    for(int i = 0; i < boxes.size(); i++)
    {
        yolov8_bbox.bbox = boxes[i];
        yolov8_bbox.confidence = confidences[i];
        yolov8_bbox.index = i;
        yolov8_bboxes.push_back(yolov8_bbox);
    }
    sort(yolov8_bboxes.begin(), yolov8_bboxes.end(), yolov8_det_comparator);
    int updated_size = yolov8_bboxes.size();
    for(int k = 0; k < updated_size; k++)
    {
        if(yolov8_bboxes[k].confidence < modelScoreThreshold)
        {
            continue;
        }

        nms_result.push_back(yolov8_bboxes[k].index);
        for(int j = k + 1; j < updated_size; j++)
        {
            float iou = box_iou(yolov8_bboxes[k].bbox, yolov8_bboxes[j].bbox);
            // float iou = box_diou(yolov8_bboxes[k].bbox, yolov8_bboxes[j].bbox);
            if(iou > modelNMSThreshold)
            {
                yolov8_bboxes.erase(yolov8_bboxes.begin() + j);
                updated_size = yolov8_bboxes.size();
                j = j -1;
            }
        }

    }
}




/***
 * caculate yolov8 pose bbox dequant value
 * int dims_cnt_1: the index for dim 1 (max index = 756)
 * int dims_cnt_2: the index for dim 2 (max index = 64 or 1)
 * TfLiteTensor* output: pointer for output tensor
 * **/
float yolov8_pose_bbox_dequant_value(int dims_cnt_1, int dims_cnt_2,TfLiteTensor* output)
{
	int value =  output->data.int8[ dims_cnt_2 + dims_cnt_1 * output->dims->data[2] ];

	float deq_value = ((float) value-(float)((TfLiteAffineQuantization*)(output->quantization.params))->zero_point->data[0]) * ((TfLiteAffineQuantization*)(output->quantization.params))->scale->data[0];
	return deq_value;
}


/***
 * caculate bbox xywh for yolov8 pose
 * int j: the index for dim 1 (max index = 756)
 * TfLiteTensor* output[7]: pointer for output tensor
 * box *bbox: output bbox result
 * float anchor_756_2[][2]: anchor matrix value
 * float *stride_756_1: stride matrix value
 * **/
// void yolov8_pose_cal_xywh(int j,TfLiteTensor* output[7], box *bbox, float anchor_756_2[][2],float *stride_756_1 )
void yolov8_pose_cal_xywh(
    int j,
    TfLiteTensor* output[7],
    box *bbox,
    float** anchor_756_2,
    float *stride_756_1
) {
    float  xywh_result[4];
    //do DFL (softmax and than do conv2d)
    int output_data_idx;
    for(int k = 0 ; k < 4 ; k++)
    {
        float tmp_arr_softmax_conv2d[16];
        float tmp_arr_softmax_conv2d_result=0;
        for(int i = 0 ; i < 16 ; i++)
        {
            float tmp_result = 0;
            if(j<576)
            {
                output_data_idx = 1;
                tmp_result = yolov8_pose_bbox_dequant_value(j, k * 16 + i,output[output_data_idx]);
            }
            else if(j<720)
            {
                output_data_idx = 0;
                tmp_result = yolov8_pose_bbox_dequant_value(j-576, k*16+i,output[output_data_idx]);
            }
            else
            {
                output_data_idx = 5;
                tmp_result = yolov8_pose_bbox_dequant_value(j-720, k*16+i,output[output_data_idx]);
            }
            tmp_arr_softmax_conv2d[i] = tmp_result;
            // tmp_arr_softmax_conv2d[i] = outputs_data_756_64[j][k*16+i];
            // if(j==0)
            // {
            //     printf("outputs_data_756_64[%d][%d]: %f\r\n",j,k*16+i,outputs_data_756_64[j][k*16+i]);
            // }
        }
        softmax(tmp_arr_softmax_conv2d,16);

        for(int i = 0 ; i < 16 ; i++)
        {

            tmp_arr_softmax_conv2d_result = tmp_arr_softmax_conv2d_result + tmp_arr_softmax_conv2d[i] * i;
            // if(j==0)printf("tmp_arr_softmax_conv2d[%d]: %f\r\n",i,tmp_arr_softmax_conv2d[i]);
        }

        xywh_result[k] = tmp_arr_softmax_conv2d_result;

    }

    /**dist2bbox * stride start***/
    float x1 = anchor_756_2[j][0] -  xywh_result[0];
    float y1 = anchor_756_2[j][1] -  xywh_result[1];

    float x2 = anchor_756_2[j][0] +  xywh_result[2];
    float y2 = anchor_756_2[j][1] +  xywh_result[3];

    float cx = (x1 + x2)/2.;
    float cy = (y1 + y2)/2.;
    float w = x2 - x1;
    float h = y2 - y1;

    xywh_result[0] = cx * stride_756_1[j];
    xywh_result[1] = cy * stride_756_1[j];
    xywh_result[2] = w * stride_756_1[j];
    xywh_result[3] = h * stride_756_1[j];

    bbox->x = xywh_result[0] - (0.5 * xywh_result[2]);
    bbox->y = xywh_result[1] - (0.5 * xywh_result[3]);
    bbox->w = xywh_result[2];
    bbox->h = xywh_result[3];
    return ;
}
///////////////////////////////////////////////////////////////

    /**
    * @brief           Helper function to load the current image into the input
    *                  tensor.
    * @param[in]       imIdx         Image index (from the pool of images available
    *                                to the application).
    * @param[out]      inputTensor   Pointer to the input tensor to be populated.
    * @return          true if tensor is loaded, false otherwise.
    **/
    static bool LoadImageIntoTensor(uint32_t imIdx, TfLiteTensor* inputTensor);

    /**
     * @brief           Helper function to increment current image index.
     * @param[in,out]   ctx   Pointer to the application context object.
     **/
    static void IncrementAppCtxImageIdx(ApplicationContext& ctx);

    /**
     * @brief           Helper function to set the image index.
     * @param[in,out]   ctx   Pointer to the application context object.
     * @param[in]       idx   Value to be set.
     * @return          true if index is set, false otherwise.
     **/
    static bool SetAppCtxImageIdx(ApplicationContext& ctx, uint32_t idx);
    /**
     * @brief           Helper function to convert a UINT8 image to INT8 format.
     * @param[in,out]   data            Pointer to the data start.
     * @param[in]       kMaxImageSize   Total number of pixels in the image.
     **/
    static void ConvertImgToInt8(void* data, size_t kMaxImageSize);
    /**
     * @brief           Draw boxes directly on the LCD for all detected objects.
     * @param[in]       platform           Reference to the hal platform object.
     * @param[in]       results            Vector of detection results to be displayed.
     * @param[in]       imageStartX        X coordinate where the image starts on the LCD.
     * @param[in]       imageStartY        Y coordinate where the image starts on the LCD.
     * @param[in]       imgDownscaleFactor How much image has been downscaled on LCD.
     **/
    static void DrawDetectionBoxes(hal_platform& platform,const std::vector<object_detection::DetectionResult>& results,
                                   uint32_t imgStartX,
                                   uint32_t imgStartY,
                                   uint32_t imgDownscaleFactor);

    static void DrawDetectionPose(hal_platform& platform,const std::vector<detection_yolov8_pose>& results_yolov8_pose,
                                   uint32_t imgStartX,
                                   uint32_t imgStartY,
                                   uint32_t imgDownscaleFactor,
                                   float modelScoreThreshold);
    /**
     * @brief           Presents inference results along using the data presentation
     *                  object.
     * @param[in]       platform           Reference to the hal platform object.
     * @param[in]       results            Vector of detection results to be displayed.
     * @return          true if successful, false otherwise.
     **/
    static bool PresentInferenceResult(hal_platform& platform,const std::vector<object_detection::DetectionResult>& results);
    /* Image inference classification handler. */
    bool ClassifyImageHandler(ApplicationContext& ctx, uint32_t imgIndex, bool runAll)
    {
        auto& platform = ctx.Get<hal_platform&>("platform");
        auto& profiler = ctx.Get<Profiler&>("profiler");

        constexpr uint32_t dataPsnImgDownscaleFactor = 1;
        constexpr uint32_t dataPsnImgStartX = 10;
        constexpr uint32_t dataPsnImgStartY = 35;
        //constexpr uint32_t dataPsnImgStartX = 10;
        // constexpr uint32_t dataPsnImgStartY = 0;

        constexpr uint32_t dataPsnTxtInfStartX = 150;
        constexpr uint32_t dataPsnTxtInfStartY = 40;

        auto& model = ctx.Get<Model&>("model");

        /* If the request has a valid size, set the image index. */
        if (imgIndex < NUMBER_OF_FILES) {
            if (!SetAppCtxImageIdx(ctx, imgIndex)) {
                return false;
            }
        }
        if (!model.IsInited()) {
            printf_err("Model is not initialised! Terminating processing.\n");
            return false;
        }

        auto curImIdx = ctx.Get<uint32_t>("imgIndex");
        const size_t numOutputs = model.GetNumOutputs();
        info("Num of outputs: %zu\n",numOutputs);
        TfLiteTensor* output[7];
        for(int i = 0; i < 7;i++)
        {
            output[i] = model.GetOutputTensor(i);
        }


        TfLiteTensor* inputTensor = model.GetInputTensor(0);

        if (!inputTensor->dims) {
            printf_err("Invalid input tensor dims\n");
            return false;
        } else if (inputTensor->dims->size < 3) {
            printf_err("Input tensor dimension should be >= 3\n");
            return false;
        }

        TfLiteIntArray* inputShape = model.GetInputShape(0);

        const uint32_t nCols = inputShape->data[arm::app::Yolov8npose::ms_inputColsIdx];
        const uint32_t nRows = inputShape->data[arm::app::Yolov8npose::ms_inputRowsIdx];
        const uint32_t nChannels = inputShape->data[arm::app::Yolov8npose::ms_inputChannelsIdx];
        info("tensor input:col:%d, row:%d, ch:%d\n", nCols,nRows,nChannels);
        std::vector<object_detection::DetectionResult> results;

        do {
            platform.data_psn->clear(COLOR_BLACK);

            /* Copy over the data. */
            LoadImageIntoTensor(ctx.Get<uint32_t>("imgIndex"), inputTensor);

            /* Display this image on the LCD. */
            const uint8_t* image = get_img_array(ctx.Get<uint32_t>("imgIndex"));
            platform.data_psn->present_data_image(
                (uint8_t*) image,
                nCols, nRows, nChannels,
                dataPsnImgStartX, dataPsnImgStartY, dataPsnImgDownscaleFactor);

            /* If the data is signed. */
            if (model.IsDataSigned()) {
                ConvertImgToInt8(inputTensor->data.data, inputTensor->bytes);
            }
            printf("\n");
            /* Run inference over this image. */
            info("Running YOLO Pose v8 192x192x3 model inference on image %" PRIu32 " => %s\n", ctx.Get<uint32_t>("imgIndex"),
                get_filename(ctx.Get<uint32_t>("imgIndex")));

            if (!RunInference(model, profiler)) {
                return false;
            }






            float output_scale[7];
            int output_zeropoint[7];
            int output_size[7];
            for(int i = 0; i < 7;i++)
            {
                // printf("output[%d]->dims->size: %d\r\n",i,output[i]->dims->size);
                output_scale[i] = ((TfLiteAffineQuantization*)(output[i]->quantization.params))->scale->data[0];
                output_zeropoint[i] = ((TfLiteAffineQuantization*)(output[i]->quantization.params))->zero_point->data[0];
                output_size[i] = output[i]->bytes;
                printf("output_scale[%d]: %f\r\n",i,output_scale[i]);
            }
            //////////////////////////////////////////////////////////////////////////////////// split output yolov8 post-proccessing

            ///// construct stride matrix
            printf("construct stride matrix start\r\n");
            // float stride_756_1[756];
            float* stride_756_1;
            stride_756_1 = (float*)calloc(756, sizeof(float));
            int stride = 8;
            int start_stride_step = 0;
            int max_stride_step = pow((INPUT_IMAGE_SIZE/stride),2);
            for(int j=0;j<3;j++)
            {
                if(j==1)
                {
                    stride = 16;
                    start_stride_step = max_stride_step;
                    max_stride_step = start_stride_step + pow((INPUT_IMAGE_SIZE/stride),2);
                    printf("start_stride_step: %d, max_stride_step: %d\r\n",start_stride_step,max_stride_step);
                }
                else if(j==2)
                {
                    stride = 32;
                    start_stride_step = max_stride_step;
                    max_stride_step = start_stride_step + pow((INPUT_IMAGE_SIZE/stride),2);
                    printf("start_stride_step: %d, max_stride_step: %d\r\n",start_stride_step,max_stride_step);
                }
                else
                {
                    printf("start_stride_step: %d, max_stride_step: %d\r\n",start_stride_step,max_stride_step);
                    ////initial value
                }
                for(int i = start_stride_step;i < max_stride_step;i++)
                {
                    stride_756_1[i] = stride;
                    // printf("%d %f ",i,stride_756_1[i]);
                }

            }
            printf("construct stride matrix done\r\n");
            ///// construct stride matrix
             ///// construct anchor matrix
            printf("construct anchor matrix start\r\n");
            // float anchor_756_2[756][2];
            float ** anchor_756_2;
            anchor_756_2 = (float**)calloc(756, sizeof(float *));
            for(int i=0;i<756;i++)
            {
                anchor_756_2[i] = (float*)calloc(2, sizeof(float));
            }
            stride = 8;
            start_stride_step = 0;
            max_stride_step = pow((INPUT_IMAGE_SIZE/stride),2);
            for(int j=0;j<3;j++)
            {
                if(j==1)
                {
                    stride = 16;
                    start_stride_step = max_stride_step;
                    max_stride_step = start_stride_step + pow((INPUT_IMAGE_SIZE/stride),2);
                    printf("start_stride_step: %d, max_stride_step: %d\r\n",start_stride_step,max_stride_step);
                }
                else if(j==2)
                {
                    stride = 32;
                    start_stride_step = max_stride_step;
                    max_stride_step = start_stride_step + pow((INPUT_IMAGE_SIZE/stride),2);
                    printf("start_stride_step: %d, max_stride_step: %d\r\n",start_stride_step,max_stride_step);
                }
                else
                {
                    printf("start_stride_step: %d, max_stride_step: %d\r\n",start_stride_step,max_stride_step);
                    ////initial value
                }
                float strart_anchor_value = 0.5;
                int max_anchor_value = (INPUT_IMAGE_SIZE/stride);
                float anchor_step_value = 0.;
                float anchor_2_step_value = -1.;
                for(int i = start_stride_step;i < max_stride_step;i++)
                {
                    if((i%max_anchor_value)==0)
                    {
                        strart_anchor_value = 0.5;
                        anchor_step_value = 0.;
                        anchor_2_step_value++;
                        // printf("\r\n");
                    }

                    anchor_756_2[i][0] = strart_anchor_value + (anchor_step_value++);
                    anchor_756_2[i][1] = strart_anchor_value + anchor_2_step_value;
                    // printf("%f ",anchor_756_2[i][0]);
                    // printf("%d %f ",i,anchor_756_2[i][1]);
                }
            }
            printf("construct anchor matrix done\r\n");


            ////////////////////////// caculate key-point start
            ////////////////////////// caculate key-point end
            int input_w=192;
            int input_h=192;
            // init postprocessing
            int num_classes = 1;


            // end init
            ///////////////////////
            // start postprocessing
            int nboxes=0;
            // float modelScoreThreshold = .25;
            // float modelNMSThreshold = .45;
            // float modelScoreThreshold = .5;
            // float modelNMSThreshold = .45;

            float modelScoreThreshold = .4;
            float modelNMSThreshold = .45;
            int image_width = input_w;
            int image_height =  input_h;

            std::vector<int> class_ids;
            std::vector<float> confidences;
            std::vector<box> boxes;
            std::vector< struct_human_pose_17> kpts_vector;
            int output_data_idx;
            for(int dims_cnt_1=0;dims_cnt_1<756;dims_cnt_1++)
            {
                //////conferen ok
                // float maxScore = sigmoid(outputs_data_756_1[dims_cnt_1]);// the first four indexes are bbox information
                float maxScore = 0;

                float tmp_result = 0;
                if(dims_cnt_1<576)
                {
                    output_data_idx = 4;
                    maxScore = sigmoid(yolov8_pose_bbox_dequant_value(dims_cnt_1, 0,output[output_data_idx]));
                }
                else if(dims_cnt_1<720)
                {
                    output_data_idx = 6;
                    maxScore = sigmoid(yolov8_pose_bbox_dequant_value(dims_cnt_1-576, 0, output[output_data_idx]));
                }
                else
                {
                    output_data_idx = 2;
                    maxScore = sigmoid(yolov8_pose_bbox_dequant_value(dims_cnt_1-720, 0,output[output_data_idx]));
                }
                int maxClassIndex = 0;
                // if (maxScore >= 0.25)
                if (maxScore >= modelScoreThreshold)
                {
                    box bbox;
                    yolov8_pose_cal_xywh(dims_cnt_1, output, &bbox, anchor_756_2, stride_756_1 );










                    boxes.push_back(bbox);
                    class_ids.push_back(maxClassIndex);
                    confidences.push_back(maxScore);

                    // printf("idx: %d,bbox.x: %f, bbox.y: %f, bbox.w: %f , bbox.h: %f\r\n",dims_cnt_2,bbox.x,bbox.y,bbox.w,bbox.h);
                    struct_human_pose_17 kpts;
                    for(int k = 0 ; k < 17 ; k++)
                    {
                        kpts.hpr[k].x = yolov8_pose_key_pts_dequant_value(dims_cnt_1,k*3 , output[3],anchor_756_2[dims_cnt_1][0],anchor_756_2[dims_cnt_1][1],stride_756_1[dims_cnt_1]);
                        kpts.hpr[k].y = yolov8_pose_key_pts_dequant_value(dims_cnt_1,k*3+1 , output[3],anchor_756_2[dims_cnt_1][0],anchor_756_2[dims_cnt_1][1],stride_756_1[dims_cnt_1]);
                        kpts.hpr[k].score = yolov8_pose_key_pts_dequant_value(dims_cnt_1,k*3+2 , output[3],anchor_756_2[dims_cnt_1][0],anchor_756_2[dims_cnt_1][1],stride_756_1[dims_cnt_1]);
                        // printf("idx: %d,kpts[%d] x: %d, y: %d, score: %f\r\n",dims_cnt_2,k,kpts.hpr[k].x,kpts.hpr[k].y,kpts.hpr[k].score);
                    }
                    kpts_vector.push_back(kpts);
                    // printf("\r\n");
                }
            }
            printf("boxes.size(): %d\r\n",boxes.size());

            /**
             * do nms
             * **/

            std::vector<int> nms_result;
            yolov8_NMSBoxes(boxes, confidences, modelScoreThreshold, modelNMSThreshold, nms_result);
            std::vector<object_detection::DetectionResult> results;
            std::vector<detection_yolov8_pose> results_yolov8_pose;
            for (int i = 0; i < nms_result.size(); i++)
            {
                int idx = nms_result[i];

                object_detection::DetectionResult temp_result;
                temp_result.m_normalisedVal = confidences[idx];
                temp_result.m_x0 = boxes[idx].x;
                temp_result.m_y0 = boxes[idx].y;
                temp_result.m_w = boxes[idx].w;
                temp_result.m_h = boxes[idx].h;

                detection_yolov8_pose temp_yolov8_pose_result;
                temp_yolov8_pose_result.bbox = boxes[idx];
                temp_yolov8_pose_result.confidence = confidences[idx];
                for(int k = 0 ; k < 17 ; k++)
                {
                    temp_yolov8_pose_result.hpr[k].x = kpts_vector[idx].hpr[k].x;
                    temp_yolov8_pose_result.hpr[k].y = kpts_vector[idx].hpr[k].y;
                    temp_yolov8_pose_result.hpr[k].score = kpts_vector[idx].hpr[k].score;
                    printf("idx: %d,kpts[%d] x: %d, y: %d, score: %f\r\n",idx,k,kpts_vector[idx].hpr[k].x,kpts_vector[idx].hpr[k].y,kpts_vector[idx].hpr[k].score);

                    if( temp_yolov8_pose_result.hpr[k].x>= input_w) temp_yolov8_pose_result.hpr[k].x= input_w-1;
                    if( temp_yolov8_pose_result.hpr[k].y >= input_h)temp_yolov8_pose_result.hpr[k].y= input_h-1;
                    #if 0
                    ////resize to original image size
                    int o_image_w = 320;
                    int o_image_h = 240;
                    temp_yolov8_pose_result.hpr[k].x = temp_yolov8_pose_result.hpr[k].x / (float)input_w * (float) o_image_w;
                    temp_yolov8_pose_result.hpr[k].y = temp_yolov8_pose_result.hpr[k].y / (float)input_h * (float) o_image_h;
                    ///resize to original image size
                    #endif
                }


                results_yolov8_pose.push_back(temp_yolov8_pose_result);

                // results.push_back(temp_result);
                results.push_back(object_detection::DetectionResult(class_ids[idx],confidences[idx],boxes[idx].x,boxes[idx].y,boxes[idx].w,boxes[idx].h));
                printf("detect object[%d]: %s confidences: %f\r\n",i, coco_classes[class_ids[idx]].c_str(),confidences[idx]);
                // float iou = box_iou(boxes[0], boxes[idx]);
                //  printf("iou: %f modelNMSThreshold: %f\r\n",iou, modelNMSThreshold);
            }

            // for(int i=0;i<results_yolov8_pose.size();i++)
            // {
            //     for(int k = 0 ; k < 17 ; k++)
            //     {

            //         printf("idx: %d,kpts[%d] x: %d, y: %d, score: %f\r\n",i,k,results_yolov8_pose[i].hpr[k].x,results_yolov8_pose[i].hpr[k].y,results_yolov8_pose[i].hpr[k].score);
            //     }

            // }

            DrawDetectionBoxes(platform,results, dataPsnImgStartX, dataPsnImgStartY, dataPsnImgDownscaleFactor);
            DrawDetectionPose(platform,results_yolov8_pose, dataPsnImgStartX, dataPsnImgStartY, dataPsnImgDownscaleFactor, 0.5);
            printf("nms_result.size():%d\r\n",nms_result.size());

            free(stride_756_1);
            for(int i=0;i<756;i++)
            {
                free(anchor_756_2[i]);
            }
            free(anchor_756_2);
            /////////////////////////////////////
            // //////////////////////////////////////////
#if VERIFY_TEST_OUTPUT
            arm::app::DumpTensor(outputTensor);
#endif /* VERIFY_TEST_OUTPUT */

            if (!PresentInferenceResult(platform,results)) {
                return false;
            }

            profiler.PrintProfilingResult();

            IncrementAppCtxImageIdx(ctx);

        } while (runAll && ctx.Get<uint32_t>("imgIndex") != curImIdx);

        return true;
    }
    static bool LoadImageIntoTensor(uint32_t imIdx, TfLiteTensor* inputTensor)
    {
        const size_t copySz = inputTensor->bytes < IMAGE_DATA_SIZE ?
                              inputTensor->bytes : IMAGE_DATA_SIZE;
        const uint8_t* imgSrc = get_img_array(imIdx);
        if (nullptr == imgSrc) {
            printf_err("Failed to get image index %" PRIu32 " (max: %u)\n", imIdx,
                       NUMBER_OF_FILES - 1);
            return false;
        }
        info("inputTensor->dims: %d\n",inputTensor->dims->size );
        info("inputTensor->bytes: %d\n",inputTensor->bytes);
        info("IMAGE_DATA_SIZE: %d\n",copySz);
        memcpy(inputTensor->data.data, imgSrc, copySz);
        debug("Image %" PRIu32 " loaded\n", imIdx);
        return true;
    }

    static void IncrementAppCtxImageIdx(ApplicationContext& ctx)
    {
        auto curImIdx = ctx.Get<uint32_t>("imgIndex");

        if (curImIdx + 1 >= NUMBER_OF_FILES) {
            ctx.Set<uint32_t>("imgIndex", 0);
            return;
        }
        ++curImIdx;
        ctx.Set<uint32_t>("imgIndex", curImIdx);
    }

    static bool SetAppCtxImageIdx(ApplicationContext& ctx, uint32_t idx)
    {
        if (idx >= NUMBER_OF_FILES) {
            printf_err("Invalid idx %" PRIu32 " (expected less than %u)\n",
                       idx, NUMBER_OF_FILES);
            return false;
        }
        ctx.Set<uint32_t>("imgIndex", idx);
        return true;
    }

    static bool PresentInferenceResult(hal_platform& platform,const std::vector<object_detection::DetectionResult>& results)
    {
        platform.data_psn->set_text_color(COLOR_GREEN);

        /* If profiling is enabled, and the time is valid. */
        info("Final results:\n");
        info("Total number of inferences: 1\n");

        for (uint32_t i = 0; i < results.size(); ++i) {
            info("%" PRIu32 ") (%f) -> %s {x=%d,y=%d,w=%d,h=%d}\n", i,
                results[i].m_normalisedVal, "Detection box:",
                results[i].m_x0, results[i].m_y0, results[i].m_w, results[i].m_h );
        }

        return true;
    }

    static void ConvertImgToInt8(void* data, const size_t kMaxImageSize)
    {
        auto* tmp_req_data = (uint8_t*) data;
        auto* tmp_signed_req_data = (int8_t*) data;

        for (size_t i = 0; i < kMaxImageSize; i++) {
            tmp_signed_req_data[i] = (int8_t) (
                (int32_t) (tmp_req_data[i]) - 128);
        }
    }
    static void DrawDetectionBoxes(hal_platform& platform,const std::vector<object_detection::DetectionResult>& results,
                                   uint32_t imgStartX,
                                   uint32_t imgStartY,
                                   uint32_t imgDownscaleFactor)
    {
        uint32_t lineThickness = 1;
        int result_idx = 0;
        for (const auto& result: results) {
            //std::string str_inf = std::to_string(result.m_class);
            std::string str_inf = "ID:"+std::to_string(result_idx);//coco_classes[result.m_class];
            result_idx++;
            platform.data_psn->present_data_text(str_inf.c_str(),str_inf.size(),imgStartX + (result.m_x0+1)/imgDownscaleFactor,
                    imgStartY + (result.m_y0+1)/imgDownscaleFactor,false);
            /* Top line. */
            platform.data_psn->present_box(imgStartX + result.m_x0/imgDownscaleFactor,
                    imgStartY + result.m_y0/imgDownscaleFactor,
                    result.m_w/imgDownscaleFactor, lineThickness, COLOR_RED);
            /* Bot line. */
            platform.data_psn->present_box(imgStartX + result.m_x0/imgDownscaleFactor,
                    imgStartY + (result.m_y0 + result.m_h)/imgDownscaleFactor - lineThickness,
                    result.m_w/imgDownscaleFactor, lineThickness, COLOR_RED);

            /* Left line. */
            platform.data_psn->present_box(imgStartX + result.m_x0/imgDownscaleFactor,
                    imgStartY + result.m_y0/imgDownscaleFactor,
                    lineThickness, result.m_h/imgDownscaleFactor, COLOR_RED);
            /* Right line. */
            platform.data_psn->present_box(imgStartX + (result.m_x0 + result.m_w)/imgDownscaleFactor - lineThickness,
                    imgStartY + result.m_y0/imgDownscaleFactor,
                    lineThickness, result.m_h/imgDownscaleFactor, COLOR_RED);
        }
    }
    static void DrawDetectionPose(hal_platform& platform,const std::vector<detection_yolov8_pose>& results_yolov8_pose,
                                   uint32_t imgStartX,
                                   uint32_t imgStartY,
                                   uint32_t imgDownscaleFactor,
                                   float modelScoreThreshold)
    {
        uint32_t lineThickness = 3;

        for (int i = 0 ; i < results_yolov8_pose.size();i++) {
            for(int k=0;k<17;k++)
            {
                if(results_yolov8_pose[i].hpr[k].score <= modelScoreThreshold)continue;
                platform.data_psn->present_box(imgStartX + results_yolov8_pose[i].hpr[k].x/imgDownscaleFactor,
                    imgStartY + results_yolov8_pose[i].hpr[k].y/imgDownscaleFactor,
                    lineThickness, lineThickness, COLOR_YELLOW);
            }
        }


    }
} /* namespace app */
} /* namespace arm */
