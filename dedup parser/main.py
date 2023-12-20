import debug_record_parser
import cv2 as cv
import matplotlib.pyplot as plt

path_to_file = "G:\My Drive\logs\geku/2023/22nd June\debug_record-1OIQZ7-2023-06-22-14-55-51\mnt\capture\debug_record"
file_name = "debug_inference_out.json"

full_path = path_to_file + "/" + file_name

if __name__ == '__main__':
    parser = debug_record_parser.DebugRecordParser(full_path)
    bboxes = parser.format_bboxes()
    stats = parser.calculate_stats()

    # Extract the material names and occurrence values from the stats dictionary
    material_names = list(stats.keys())
    occurrence_values = list(stats.values())

    # Plotting the bar graph
    bars = plt.bar(material_names, occurrence_values)
    plt.xlabel("Material Name")
    plt.ylabel("Occurrence")
    plt.title("Material Occurrence Statistics")

    # Add value labels on top of each bar
    for bar in bars:
        yval = bar.get_height()
        plt.text(bar.get_x() + bar.get_width() / 2, yval, yval, ha='center', va='bottom')


    # Rotate the x-axis labels
    plt.xticks(rotation=50)
    # Adjust figure size to accommodate the rotated labels
    plt.tight_layout()

    plt.show()

    print(parser.bboxes)