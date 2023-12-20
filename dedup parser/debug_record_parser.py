import json
import cv2 as cv


class DebugRecordParser:
    def __init__(self, debug_record_path):
        self.debug_record_path = debug_record_path
        self.bboxes = []
        self.img_height = 0
        self.img_width = 0
        # Format the debug record path
        self.debug_log_filename = debug_record_path.split('/')[-1]
        self.debug_record_path = debug_record_path.replace(self.debug_log_filename, '')
        # Keep count of what we see
        self.material_poll = {}


    def parse_file(self):
        path = self.debug_record_path + "/" + self.debug_log_filename
        with open(path, 'r') as f:
            for line in f:
                # Parse each line as JSON
                json_data = json.loads(line)
                self.bboxes.append(json_data)
        return self.bboxes

    def get_image_data(self):
        if len(self.bboxes) != 0:
            self.parse_file()
            img_path_name = self.bboxes[0]['image_file']
            img_path_name = img_path_name.split('/')[-1]
            path_to_image = self.debug_record_path + "/" + img_path_name
            # load image
            img = cv.imread(path_to_image)
            self.img_height = img.shape[0]
            self.img_width = img.shape[1]

    def format_bboxes(self):
        if len(self.bboxes) == 0:
            self.parse_file()
        if self.img_height == 0 or self.img_width == 0:
            self.get_image_data()
        bboxes_len = len(self.bboxes)
        for k in range(bboxes_len):
            bbox = self.bboxes[k]
            # Format the bounding boxes
            detections = bbox["bboxes"]
            scaled_bboxes = []
            num_detections = len(detections)
            if detections is not None:
                for i in range(num_detections):
                    detection = detections[i]
                    bbox_s = {}
                    # Format the bounding boxes
                    bbox_s["x1"] = int(detection[0] * self.img_width)
                    bbox_s["y1"] = int(detection[1] * self.img_height)
                    bbox_s["x2"] = int(detection[2] * self.img_width)
                    bbox_s["y2"] = int(detection[3] * self.img_height)
                    bbox_s["conf"] = bbox["vals"]["confs"][i]
                    bbox_s["class"] = bbox["labels"][i]
                    scaled_bboxes.append(bbox_s)
            self.bboxes[k]["objects"] = scaled_bboxes
        return self.bboxes

    def calculate_stats(self):
        if len(self.bboxes) == 0:
            self.format_bboxes()
        num_frames = len(self.bboxes)
        num_objects = 0
        for frame in self.bboxes:
            objects = frame["objects"]
            num_objects += len(objects)
            # check if objects["class"] is in material poll
            if objects is not None:
                for obj in objects:
                    obj_class = obj["class"]
                    if obj_class in self.material_poll:
                        self.material_poll[obj_class] += 1
                    else:
                        self.material_poll[obj_class] = 1
        return self.material_poll
