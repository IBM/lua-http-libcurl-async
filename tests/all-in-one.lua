#!/usr/bin/lua
package.cpath = package.cpath..";/usr/lib/lua/5.1/?.so;"
local paths = {
  package.path -- the good ol' package.path
}
package.path = table.concat(paths, ";")
local async_http = require("lua_async_http")

local function parse_response(res)
  return res.url, res.response_status, res.response_body, res.response_headers, res.response_error
end

-- request
local requests = {
  -- HTTPS POST (post_params) 
  {
    -- the 'name' is the key we use to retrive the data from
    -- at the response stage.
    name = "test1",
    -- simply, a valid http/https url
    url = "https://www.example.com/",
    -- When using POST method, we can use two delivery options, 
    -- 1. post_params, 2. data. WE CANNOT USE THEM BOTH!!!
    method = "POST",  
    post_params = "first_var=123&second_var=456",

    -- Each header declared separately as table.
    headers = {
      {["Content-Type"] = "application/json"}, 
      {["SOME-OTHER-HEADER"] = "Some_other_value"}
    },
    -- request timeout in seconds
    timeout = 7
  },
  -- HTTPS POST (data) with certifications
  {
    name = "test2",
    url = "https://www.example.com/",
    method = "POST",
    -- When using POST method, we can use two delivery options, 
    -- 1. post_params, 2. data. WE CANNOT USE THEM BOTH!!!
    data = [[
      {
        "some": "data"
      }
    ]],
    headers = {
      {["User-Agent"] = "Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/51.0.2704.103 Safari/537.36"},
      {["Content-Type"] = "application/json"},
    },
    timeout = 4,
    certificate = "/path/to/cert.pem",  -- path to public key
    cafile = "/path/to/ca.crt", -- path to cafile
    key = "/path/to/key.pem", -- path to private key
    -- password = "certificate_password_if_there_is",

    -- We can use debug to simply debug our request headers. (boolean)
    debug = 0,
    verify_host = 1,
    verify_peer = 1,
    timeout = 8,
  },
  -- HTTP GET
  {
    name = "test3",
    url = "http://www.example.com",
    method = "GET",
    headers = {
      {["User-Agent"] = "Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/51.0.2704.103 Safari/537.36"},
    },
    timeout = 5
  },
  -- HTTP POST
  {
    name = "test4",
    url = "http://www.example.com",
    method = "POST",
    data = [[
      {
        "watch": {
          "this": true,
          "in": true,
          "action": true
        }
      }
    ]],
    headers = {
      {["User-Agent"] = "Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/51.0.2704.103 Safari/537.36"},
    },
    timeout = 5
  },
  -- PUT REQUEST
  {
    -- the 'name' is the key we use to retrive the data from
    -- at the response stage.
    name = "test5",
    -- simply, a valid http/https url
    url = "https://www.example.com",
    -- When using PUT method, we must use data field
    -- for data delivering 
    method = "PUT",  
    data = [[{
      "event": {
      }
    }]],
    debug = 0,
    -- Each header declared separately as table.
    headers = {
      {["Content-Type"]         = "application/json"},
      {["Some-Header"]          = "Some-Value"}
    },
    certificate = "/path/to/cert.pem",  -- path to public key
    cafile = "/path/to/ca.crt", -- path to cafile
    key = "/path/to/key.pem", -- path to private key
    -- request timeout in seconds
    timeout = 3.5
  },
}

-- the responses returned in the following structure
--   name = {
--     url = "https://www.example.com/",
--     response_status = 200,
--     response_body = "RES BODY",
--     response_headers = {
--      ["header-type"] = "header value",
--      ...
--     }
--     resposne_error = "..."
--   },...

local ok, res = pcall(function()
  return async_http.request(requests)
end)

if not ok then
  print("Error occurred: ", res)
  return
end

local url, status, res_body, res_headers, err
for i=1, #requests do
  url, status, res_body, res_headers, err = parse_response(res[string.format("test%s",i)])
  print(url, status, res_headers, err)
end

