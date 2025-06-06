#!/bin/bash

PORT=8000

# Start background POST flood
echo "Starting POST flood..."
(while true; do curl -s -X POST http://localhost:$PORT/ > /dev/null; done) &

POST_PID=$!

# Give the POST flood a second to ramp up
sleep 2

# Now run a GET and time it
echo "Running GET with writer priority test..."
time curl -v http://localhost:$PORT/home.html

# Kill the POST flood
kill $POST_PID

echo "POST flood stopped."
