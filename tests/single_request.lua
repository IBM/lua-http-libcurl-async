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
    url = "http://www.example.com",
    -- When using POST method, we can use two delivery options, 
    -- 1. post_params, 2. data. WE CANNOT USE THEM BOTH!!!
    method = "POST",
    
    data = require("cjson").encode({["x"]= string.rep("aaaaaaaaaaa", 10000)}),
    -- Each header declared separately as table.
    headers = {
      {["Content-Type"] = "application/json"}, 
      {["SOME-OTHER-HEADER"] = "Some_other_value"}
    },
    -- request timeout in seconds
    timeout = 5.5,
    debug = 1,
    --expectations = 1000
  }
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

