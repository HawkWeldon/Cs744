#!/usr/bin/env bash

BASE_URL="http://127.0.0.1:8080"

passed=0
failed=0

test_request()
{
    local name="$1"
    local expected="$2"
    shift 2

    local actual

    actual=$(curl -s -o /dev/null -w "%{http_code}" "$@")

    if [ "$actual" = "$expected" ]
    then
        printf "PASS: %-30s HTTP %s\n" "$name" "$actual"
        ((passed++))
    else
        printf "FAIL: %-30s Expected %s, got %s\n" "$name" "$expected" "$actual"
        ((failed++))
    fi
}

echo "=============== cflask tests ==============="

test_request "Root" 200 "$BASE_URL/"
test_request "Square" 200 "$BASE_URL/square?num=5"
test_request "Cube" 200 "$BASE_URL/cube?num=3"
test_request "Hello World" 200 "$BASE_URL/helloworld?str=Hello"
test_request "Ping Pong" 200 "$BASE_URL/pingpong?Val=10"
test_request "Prime" 200 "$BASE_URL/arithmetic/prime?num=7"
test_request "Fibonacci" 200 "$BASE_URL/arithmetic/fibonacci?num=8"

test_request "Multiple parameters" 200 "$BASE_URL/square?unused=10&num=5"
test_request "URL decoding" 200 "$BASE_URL/helloworld?str=Hello%20World"

test_request "POST square" 200 \
    -X POST \
    -H "Content-Type: application/x-www-form-urlencoded" \
    -d "num=5" \
    "$BASE_URL/square"

test_request "Unknown route" 404 "$BASE_URL/does-not-exist"

test_request "Unsupported method" 405 \
    -X PUT "$BASE_URL/square?num=5"

test_request "Unsupported content type" 415 \
    -X POST \
    -H "Content-Type: application/json" \
    -d '{"num":5}' \
    "$BASE_URL/square"

test_request "Malformed URL encoding" 400 \
    "$BASE_URL/square?num=%ZZ"

echo
echo "Passed: $passed"
echo "Failed: $failed"

if [ "$failed" -ne 0 ]
then
    exit 1
fi