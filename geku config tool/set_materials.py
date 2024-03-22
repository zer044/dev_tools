# This script sets materials in registry 
# You can set this up with top-level materials and the script will 
# populate the register values for you.

import json
import sys
if(sys.platform == 'win32'):
    import winreg as wr


# Load data from json
config_data= json.load(open('material_config.json'))

model_taxonomy_list = config_data['model_taxonomy_list']
top_level_to_taxonomy_dict = config_data['top_level_to_taxonomy_dict']
object_bay1_assignment = config_data['object_bay1_assignment']
object_bay2_assignment = config_data['object_bay2_assignment']
object_bay3_assignment = config_data['object_bay3_assignment']
object_bay4_assignment = config_data['object_bay4_assignment']


object_bay1_list = []
object_bay2_list = []
object_bay3_list = []
object_bay4_list = []

for material in model_taxonomy_list:
    for i in range(len(object_bay1_assignment)):
        if object_bay1_assignment[i]:  # This checks if the list element is not empty
            if material in top_level_to_taxonomy_dict.get(object_bay1_assignment[i], []):
                object_bay1_list.append(material)

    for i in range(len(object_bay2_assignment)):
        if object_bay2_assignment[i]:  # This checks if the list element is not empty
            if material in top_level_to_taxonomy_dict.get(object_bay2_assignment[i], []):
                object_bay2_list.append(material)

    for i in range(len(object_bay3_assignment)):
        if object_bay3_assignment[i]:  # This checks if the list element is not empty
            if material in top_level_to_taxonomy_dict.get(object_bay3_assignment[i], []):
                object_bay3_list.append(material)

    for i in range(len(object_bay4_assignment)):
        if object_bay4_assignment[i]:  # This checks if the list element is not empty
            if material in top_level_to_taxonomy_dict.get(object_bay4_assignment[i], []):
                object_bay4_list.append(material)

# Write the final object_bay lists to a file for debug
object_bay_data = { "object_bay1": object_bay1_list,
        "object_bay2": object_bay2_list,
        "object_bay3": object_bay3_list,
        "object_bay4": object_bay4_list
    }
with open('object_bays.json', 'w') as f:
    json.dump(object_bay_data, f, indent=4)

# Run if windows
if (sys.platform == "win32"):
    # Set Windows Registry key values for each bay with the value as a comma-delimited string
    for bay_name, items in object_bay_data.items():
        bay_value = ",".join(items)  # Join list items into a comma-delimited string

        registry_key_path = r'SOFTWARE/VB and VBA Program Settings/PickMasterDLL/General'

        try:
            # Open the specified registry key. Create it if it does not exist.
            key = wr.OpenKey(wr.HKEY_LOCAL_MACHINE, registry_key_path, 0, wr.KEY_WRITE | wr.KEY_WOW64_64KEY)
        except FileNotFoundError:
            # If the key doesn't exist, create it
            key = wr.CreateKeyEx(wr.HKEY_LOCAL_MACHINE, registry_key_path, 0, wr.KEY_WRITE | wr.KEY_WOW64_64KEY)

        # Set the registry value for the current bay
        wr.SetValueEx(key, bay_name, 0, wr.REG_SZ, bay_value)

        # Close the key to write changes to the registry
        wr.CloseKey(key)

    # Verify
    # Open the registry key and read the values
    key = wr.OpenKey(wr.HKEY_LOCAL_MACHINE, registry_key_path, 0, wr.KEY_READ | wr.KEY_WOW64_64KEY)
    for i in range(wr.QueryInfoKey(key)[1]):
        name, value, _ = wr.EnumValue(key, i)
        print(f'{name} = {value}')
    wr.CloseKey(key)





