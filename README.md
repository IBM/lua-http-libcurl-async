# lua-async-http

lua-async-http rock, is a new lua rock written in C and based on libcurl. It allow us to make multiple http/https (with client certificate) calls in parallel (non-blocking) and wait for their responses.

In order to achieve such a behaviour, we've used the multi interface that libcurl has to offer. 


## Dependencies

The rock required the following system packages in order to compile:

 1. libcurl >= 7.34.0
 2. libcurl-devel >= 7.34.0

(TLS1.2 is supported since libcurl 7.34.0)
QA env currently has libcurl *7.61.1-7.91.amzn1* installed by default.

## Lua Usage
**Important** 
The following usage instructions are for the rock itself, and not for some lua based client.   

**Library require**
The library should be required by the caller module.
```
local async = require("lua_async_http")
```

## Request Structure
The client has only one method avilable outside, the method name is:  "request". The request method expect to get a bulked object of requests. For example:
```
{
	{request_object},
	{request_object},
	{request_object},
	...
}
```
Each ***request_object*** is a detailed request with the following optional/mandatory keys:

|key|value|type|mandatory|
|--|--|--|--|
|name|The request key (no spaces!)|string|true|
|method|GET \| POST \| PUT|string|true|
|post_params|Post parameters, only if needed (for ex: a=true&b=val). post_params and data should not be configured at once!|string|false*|
|data|The request body, in case of POST \| PUT should transfer a body data. Example: {"data":true}. data and post_params should not be configured at once!|string|false*|
|url|The request url|string|true|
|headers|A collection of headers. Example: {{["Content-Type"] = "application/json"}, {["Custom-Header"] = "custom_value"}}|collection|true|
|cafile|Path to CA PEM file|string|false|
|key|Path to private key PEM file|string|false|
|certificate|Path to public key PEM file|string|false|
|password|The TLS private key password in case PEM has it|string|false|
|verify_peer|Verify peer signature. (default: 1)|bool(1\|0)|false|
|verify_host|Verify host signature. (default: 0)|bool(1\|0)|false|
|timeout|The request timeout (in seconds. default= 8s)|number|false|
|debug|Print to stdout for debugging|bool(1\|0)|false|

(* : cannot configure at the same time)

## Requests example 
```
local async = require("lua_async_http")
local res = async.request({
	{
		name = "key_1",
		url = "http://www.example.com/send",
		method = "POST",
		data = [[{"data": true}]],
		headers = {
			{["Content-Type"] =  "application/json"},
		},
		timeout = 2.5
	},
	{
		name = "key_2",
		url = "https://www.example.com/some_ssl/send",
		method = "GET",
		cafile = "/path/to/ca.pem",
		certificate = "/path/to/cert.pem",
		key = "/path/to/key.pem"
	}
})
```

Take a look more [request examples](https://gitlab.haifa.ibm.com/pinpoint/lua_async_http/blob/master/tests/all-in-one.lua).

## Response
The response object is being returned based on the given request **name**s. According the last request example, the response should return in the following structure: 
```
{
	["key_1"] = {
		url = "http://www.example.com/send",
		response_status = "200",
		response_headers = {
			["content_type"] = "application/json"
		},
		response_body = "<HTML>...</HTML>",
		response_error = ""
	},
	["key_2"] = {
		url = "https://www.example.com/some_ssl/send",
		response_status = "0",
		response_headers = {},
		response_body = "",
		response_error = "Couldn't resolve host 'www.example.com'"
		
	}
}
```

So in order to get the values, we need to access the **res** variable (in that example) by the desired key.

```
print(res["key_1"].response_body)
-- prints: <HTML>...</HTML>
```

#### Response Types
|key|type|
|--|--|
|url|string|
|response_status|integer|
|response_headers|table|
|response_body|string|
|response_error|string|

## Things to take into considerations

 1. A bulked request error may rarely fail, therefore it must be pcalled:
	```
	local async = require("lua_async_http")
	local ok, res = pcall(function()
		return async.request({
			...
		})
	end)

	if ok then
		-- now accessing res object should be safe
	else
		-- in case of a failure, res has the failure reason
		print(string.format("failed due to: %s", res))
	end
	```
2. When using **POST** method, you must use only 1 delivery option (**data** or **post_params**).
3. The SSL filenames can have other file extension names, but they content must based on PEM.
4. In case of an error in a certain request, we returning libcurl official error message through "response_error" key.
5. In *response_headers*, we are lowercasing the key names by default.
