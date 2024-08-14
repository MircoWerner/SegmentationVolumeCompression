#!/bin/bash

data=...

./segmentationvolumes $data evaluation_cells_svo_wlod_32 --evaluate
./segmentationvolumes $data evaluation_cells_svo_wlod_1 --evaluate
./segmentationvolumes $data evaluation_cells_svo_wolod_32 --evaluate
./segmentationvolumes $data evaluation_cells_svo_wolod_1 --evaluate
./segmentationvolumes $data evaluation_cells_svdag_wlod_32 --evaluate
./segmentationvolumes $data evaluation_cells_svdag_wlod_1 --evaluate
./segmentationvolumes $data evaluation_cells_svdag_wolod_32 --evaluate
./segmentationvolumes $data evaluation_cells_svdag_wolod_1 --evaluate
./segmentationvolumes $data evaluation_cells_svdag_merged_wlod_32 --evaluate
./segmentationvolumes $data evaluation_cells_svdag_merged_wlod_1 --evaluate
./segmentationvolumes $data evaluation_cells_svdag_merged_wolod_32 --evaluate
./segmentationvolumes $data evaluation_cells_svdag_merged_wolod_1 --evaluate
./segmentationvolumes $data evaluation_cells_svdag_occupancy_field_wlod_32 --evaluate
./segmentationvolumes $data evaluation_cells_svdag_occupancy_field_wlod_1 --evaluate
./segmentationvolumes $data evaluation_cells_svdag_occupancy_field_wolod_32 --evaluate
./segmentationvolumes $data evaluation_cells_svdag_occupancy_field_wolod_1 --evaluate
./segmentationvolumes $data evaluation_cells_svdag_occupancy_field_merged_wlod_32 --evaluate
./segmentationvolumes $data evaluation_cells_svdag_occupancy_field_merged_wlod_1 --evaluate
./segmentationvolumes $data evaluation_cells_svdag_occupancy_field_merged_wolod_32 --evaluate
./segmentationvolumes $data evaluation_cells_svdag_occupancy_field_merged_wolod_1 --evaluate
