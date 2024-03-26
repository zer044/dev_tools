# Load sample images then draw bboxs and detection results on them
import json
import cv2 as cv
import os

def redraw_inference(path, inference_data):
    # Load the image
    img = cv.imread(path)
    # Draw the bounding boxes
    for i in range(len(inference_data["labels"])):
        x1 = int(inference_data["bboxes"][i][0] * img.shape[1])
        y1 = int(inference_data["bboxes"][i][1] * img.shape[0])
        x2 = int(inference_data["bboxes"][i][2] * img.shape[1])
        y2 = int(inference_data["bboxes"][i][3] * img.shape[0])
        cv.rectangle(img, (x1, y1), (x2, y2), (0, 255, 0), 2)
        cv.putText(img, inference_data["labels"][i], (x1, y1 - 10), cv.FONT_HERSHEY_SIMPLEX, 0.9, (36, 255, 12), 2)
        # Draw the confidence value "confs"
        cv.putText(img, str(inference_data["vals"]["confs"][i]), (x1, y1 - 30), cv.FONT_HERSHEY_SIMPLEX, 0.9, (36, 255, 12), 2)

    # Re-write the image to a folder in path called "labelled_images"
    image_name = os.path.basename(path)
    folder_path = os.path.join(os.path.dirname(path), "labelled_images")
    os.makedirs(folder_path, exist_ok=True)  # Create the directory if it doesn't exist
    new_path = os.path.join(folder_path, image_name)
    cv.imwrite(new_path, img)

# Load a list of image names.
sample_images_path = "sampling_record_20240325155121"
# Make a list of all files in the directory that end with .jpg
sample_images = [f for f in os.listdir(sample_images_path) if f.endswith('.jpg')]

# Load the sampling inferenc file sample_images_path\sampling_inference_out.json
sample_inference_file = sample_images_path + "/sampling_inference_out.json"
# Read a line at a time
with open(sample_inference_file, 'r') as f:
    for line in f:
        json_obj = json.loads(line)
        for img in sample_images:
            if json_obj["image_file"] == img:
                redraw_inference(sample_images_path + "/" + img, json_obj)
                break

