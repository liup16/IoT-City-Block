import json
from urllib.parse import urlencode
from urllib.request import Request, urlopen
from urllib.error import HTTPError, URLError

def get_ant():
    server = ""
    path =  ""
    url = "https://" + server + path
    api_key = ""
    headers = {'x-api-key': api_key, 'Cache-Control': "no-cache"}
    data = None
    req = Request(url, data, headers)
    try:
        response = urlopen(req)
    except HTTPError as e:
        print('The server couldn\'t fulfill the request.')
        print('Error code: ', e.code)
    except URLError as e:
        print('We failed to reach a server.')
        print('Reason: ', e.reason)
    else:
        # everything is fine
        result = response.read()
        result = json.loads(result.decode())

        busNum = result['busRecord']
        bikeNum = result['bikeRecord']
        result['busNum'] = busNum
        result['busNum'] = bikeNum
        return(result)

if __name__ == '__main__':
    print(get_ant())
