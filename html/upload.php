<?php

header("content-type: application/json");


$uploadPath = "SERVER PATH FOR UPLOAD LOCATION DOES HERE";

if(!isset($_SERVER["HTTP_API_KEY"]) || $_SERVER["HTTP_API_KEY"] != "YOUR API KEY GOES HERE")
{
	die(json_encode(array("status" => "failure", "reason" => "Authentication failure")));
}

if(!isset($_SERVER["HTTP_FILE_TYPE"]))
{
	die(json_encode(array("status" => "failure", "reason" => "Missing parameters")));
}

$fileType = strtolower($_SERVER["HTTP_FILE_TYPE"]);
$token = substr(base64_encode(sha1(mt_rand())), 0, 5);
while(file_exists("$uploadPath/$token.$fileType"))
{
        $token = substr(base64_encode(sha1(mt_rand())), 0, 5);
}

$putdata = fopen("php://input", "r");

$fp = fopen("$uploadPath/$token.$fileType", "w");

while ($data = fread($putdata, 1024))
{
	fwrite($fp, $data);
}

echo json_encode(array("status" => "success", "download-link" => "https://YOUR_SERVER.com/$token.$fileType"));

fclose($fp);
fclose($putdata);
