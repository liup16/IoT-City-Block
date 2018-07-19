from __future__ import print_function
import boto3
import json
import decimal
from boto3.dynamodb.conditions import Key, Attr
from botocore.exceptions import ClientError

region_name=''
endpoint_url=""

# Helper class to convert a DynamoDB item to JSON.
class DecimalEncoder(json.JSONEncoder):
    def default(self, o):
        if isinstance(o, decimal.Decimal):
            if abs(o) % 1 > 0:
                return float(o)
            else:
                return int(o)
        return super(DecimalEncoder, self).default(o)

def open_DB_table(table_name):
    print('Open resource')
    dynamodb = boto3.resource('dynamodb', region_name=region_name, endpoint_url=endpoint_url)
    print('Resource opened')

    table = dynamodb.Table(table_name)
    print('Table found')
    return(table)

def put_DB_item(table, action_number, action_type, info={}):
    response = table.put_item(
       Item={
            'ActionNumber': action_number,
            'ActionType': action_type,
            'info': info
        }
    )
    print("PutItem succeeded:")
    print(json.dumps(response, indent=4, cls=DecimalEncoder))

def get_DB_item(table, action_number, action_type):
    try:
        response = table.get_item(
        Key={
            'ActionNumber': action_number,
            'ActionType': action_type
        }
    )
    except ClientError as e:
        print(e.response['Error']['Message'])
        return(None)
    else:
        item = response['Item']
        print("GetItem succeeded:")
        print(json.dumps(item, indent=4, cls=DecimalEncoder))
        return(item)
