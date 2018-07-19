# Author: Patrick Aung
# Main AWS Lambda Function

import json
from get_time_temp import get_time_temp
from dynamoDB import DecimalEncoder
from dynamoDB import open_DB_table
from dynamoDB import put_DB_item
from dynamoDB import get_DB_item
from get_time_temp import get_time_temp
from get_neigbor_stats import get_ant

# Find the average of an int list
def find_avg(list):
    avg = 0
    for i in range(len(list)):
        avg += list[i]
    return avg/len(list)

# Makes index num cycle from 0-4 to update our 5-element Dyanamo DB list
def updateIndex(num):
    if num < 4:
        return num+1
    return 0

# Updates one specific index of list
def list_update(list, data_feather, index):
    print ("Changing index:", index)
    for i in range(len(list)):
        if i != index:
            list[i] = list[i]
        else:
            list[i] = data_feather
    print("updated pop_list", list)
    return list

# Main Function
def lambda_handler(event, context):
    httpRequest = event.get('requestContext').get('httpMethod')
    if httpRequest == 'POST':

        ttd = get_time_temp()
        print('Time',ttd['time'])
        print('Temp',ttd['temp'])

        ant_info = get_ant()
        bus_num = ant_info['busRecord']
        bike_num = ant_info['bikeRecord']
        print('People on Bus:', bus_num)
        print('People on Bike: ', bike_num)
        print (ant_info)

        body = json.loads(event.get('body', {})) #'body' for POST
        # Sensor values from Feather extracted to use in Lambda
        pop_from_feather = body["population"]
        temp_from_feather = body["temperature"]
        water_from_feather = body["water"]
        pot_from_feather = body["pot"]
        light_from_feather = body["light"]
        objToFeather = {}
        # Triggers to send back to Feather
        objToFeather["popTrigger"] = 0
        objToFeather["tempTrigger"] = 0
        objToFeather["waterTrigger"] = 0
        objToFeather["potTrigger"] = 0
        objToFeather["lightTrigger"] = 0
        objToFeather["numBreakIns"] = 0
        objToFeather["globalTime"] = ttd['time']
        objToFeather["busNum"] = bus_num
        objToFeather["bikeNum"] = bike_num
        objToFeather["antTrigger"] = 0
        print ('Global Time:',objToFeather["globalTime"] )

        if pop_from_feather > 3:
            objToFeather["popTrigger"] = 1
        if temp_from_feather > 28.0:
            objToFeather["tempTrigger"] = 1
        if water_from_feather > 570:
            objToFeather["waterTrigger"] = 1
        if pot_from_feather > 500:
            objToFeather["potTrigger"] = 1
        if (light_from_feather < 650 and light_from_feather != -1 and ttd['time'] <= 17.0) or ttd['time'] > 17.0:
            objToFeather["lightTrigger"] = 1
        if (bus_num + bike_num) > 40:
            objToFeather["antTrigger"] = 1

        table = open_DB_table('CityInfo')
        action_number = 1
        action_type = "cityAction"
        item = get_DB_item(table, action_number, action_type)

        # Take out previous index from DynamoDB
        index =  int(item["info"]["index"])

        # Update index by 1 (make index cycle from 0-4)
        index = updateIndex(index)

        # Turn str list to int list
        population_in_int = list(map(int, item["info"]["population"]))
        temp_in_int = list(map(float, item["info"]["temp"]))
        water_in_int = list(map(int, item["info"]["water"]))
        light_in_int = list(map(int, item["info"]["light"]))

        # Update element at index+1 of lists from DynamoDB
        pop_list = list_update(population_in_int, pop_from_feather, index)
        temp_list = list_update(temp_in_int, temp_from_feather, index)
        water_list = list_update(water_in_int, water_from_feather, index)
        light_list = list_update(light_in_int, light_from_feather, index)

        # Finding average values of updated lists
        objToFeather["avg_pop"] = int(find_avg(pop_list))
        objToFeather["avg_temp"] = round(find_avg(temp_list), 2)
        objToFeather["avg_water"] = int(find_avg(water_list))
        objToFeather["avg_light"] = int(find_avg(light_list))

        # Finding max value from list
        objToFeather["max_pop"] = max(int(s) for s in pop_list)
        objToFeather["max_temp"] = max(int(s) for s in temp_list)
        objToFeather["max_water"] = max(int(s) for s in water_list)
        objToFeather["max_light"] = max(int(s) for s in light_list)

        # Put my updated DyanmoDB items back to table as an "JSON object"
        value = {
            "index": str(index),
            "population": [str(pop_list[0]), str(pop_list[1]), str(pop_list[2]), str(pop_list[3]), str(pop_list[4])],
            "temp": [str(temp_list[0]), str(temp_list[1]), str(temp_list[2]), str(temp_list[3]), str(temp_list[4])],
            "water": [str(water_list[0]), str(water_list[1]), str(water_list[2]), str(water_list[3]), str(water_list[4])],
            "light": [str(light_list[0]), str(light_list[1]), str(light_list[2]), str(light_list[3]), str(light_list[4])]
        }

        put_DB_item(table, action_number, action_type, value)
        out = {}
        out['statusCode']=200
        out['body'] = json.dumps(objToFeather)
        return(out)

    elif httpRequest == 'GET':
        table = open_DB_table('CityInfo')
        action_number = 1
        action_type = "cityAction"
        item = get_DB_item(table, action_number, action_type)
        index =  int(item["info"]["index"])
        population_in_int = list(map(int, item["info"]["population"]))
        temperature_in_int = list(map(float, item["info"]["temp"]))
        shared_population = population_in_int[index]
        shared_temperature = temperature_in_int[index]
        out = {}
        out['statusCode'] = 200
        out['body'] = json.dumps({"message": "This is a GET method!", "shared_population": shared_population, "shared_temperature": shared_temperature})
        return (out)

    else:
        out = {}
        out['body'] = json.dumps({"message": "This is an unexpected method!"})
        return (out)
