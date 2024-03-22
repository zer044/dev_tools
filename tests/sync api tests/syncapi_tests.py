import requests
import json


class SyncAPITests:

    # Define the API endpoint URL
    unit_ip = ""
    params_endpoint = "/v2/params/values/SYNCNS"
    base_url = "http://<ip-address>:8080"
    app_id_endpoint = "/v2/params/values"
    initial_json = {}
    appid = ""

    #init
    def __init__(self, unit_ip):
        self.unit_ip = unit_ip
        self.base_url = self.base_url.replace("<ip-address>", self.unit_ip)
        self.appid = self.get_appid()

    # Function to read test data from a JSON file
    def read_test_data(self, filename):
        with open(filename, 'r') as file:
            test_data = json.load(file)
        return test_data

    def get_appid(self):
        response = requests.get(self.base_url + self.app_id_endpoint)
        data = {}
        if response.status_code == 200:
            data = json.loads(response.text)
            # find first param to start with app
            for param in data:
                if param.startswith("app"):
                    return param.split(".")[1]
        else:
            raise Exception(f"Failed to retrieve app id. Status code: {response.status_code}")


    def get_parameter(self, parameter_name):
        endpoint = self.base_url + self.app_id_endpoint + f"/app.{self.appid}.sync.{parameter_name}"
        response = requests.get(endpoint)
        if response.status_code == 200:
            return json.loads(response.text)
        else:
            raise Exception(f"Failed to retrieve parameter {parameter_name}. Status code: {response.status_code}")


    # Function to make a POST request and update parameter values
    def update_parameter(self, parameter):
        headers = {"Content-Type": "application/json"}
        endpoint = self.base_url + self.params_endpoint
        response = requests.post(endpoint, json=parameter, headers=headers)
        if response.status_code == 200:
            return True
        else:
            parameter_name = list(parameter.keys())[0]
            raise Exception(f"Failed to update parameter {parameter_name}. Status code: {response.status_code}")

    # Function to make a GET request and retrieve the initial value of a parameter
    def get_initial_value(self):
        endpoint = self.base_url + self.params_endpoint
        response = requests.get(endpoint)
        if response.status_code == 200:
            self.initial_json = json.loads(response.text)
        else:
            raise Exception(f"Failed to retrieve initial values. Status code: {response.status_code}")


    # Function to run the tests
    def run_tests(self):
        test_data = self.read_test_data("sync api tests/test_data.json")

        self.get_initial_value()

        for key, value in test_data.items():
            parameter = {key: value}
            # Update the parameter with the new value
            self.update_parameter(parameter)
            # get the parameter again
            parameter_name = list(parameter.keys())[0]
            updated_parameter = self.get_parameter(parameter_name)
            updated_value = list(updated_parameter.values())[0]
            if updated_value != value:
                raise Exception(f"Failed to update parameter {parameter_name}. Expected value: {parameter.value}. Actual value: {updated_parameter['value']}")
            else:
                print(f"Parameter {parameter_name} updated successfully")

            # Revert the parameter to the initial value
            self.update_parameter({parameter_name : self.initial_json[parameter_name]})

if __name__ == "__main__":
    sync_api_tests = SyncAPITests("10.9.0.157")
    sync_api_tests.run_tests()
