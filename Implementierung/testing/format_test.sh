#!/bin/bash

make

#rm test_images/out/valid/*pgm
#rm test_images/out/invalid/*pgm

echo "These tests are for the image read function, to verify that the function can read valid images and reject invalid images"
echo ""
echo "Tests for valid Image formats (PPM)"

# Array of test image names
declare -a images=("mandrill" "comment1" "comment2" "comment3" "comment4" "multiple" "small")

test_counter=1

for image in "${images[@]}"; do

  # Capture stderr in a variable
  err_output=$(./main.out ./testing/in/valid/${image}.ppm -o ./testing/out/valid/${image}.pgm 2>&1 >/dev/null)

  echo ""
  # Check if stderr is empty
  if [ -z "$err_output" ]; then
    echo -n "Passed - Test ${test_counter} - ${image}.ppm -o ${image}.pgm"
  else
    echo -n "Failed - Test ${test_counter} - ${image}.ppm -o ${image}.pgm"
    echo "Error Output: $err_output"
  fi
  ((test_counter++))

done

echo ""
echo ""
echo "Tests for invalid Image formats (PPM)"

declare -a images=("notEnoughPixel" "invaildFormat" "invaildComment" "negativeImageSize1" "negativeImageSize2" "missingMaxValue" "missingValues" "tooLargeMaxval" "overflow" "invalidWhiteSpace")
test_counter=1

for image in "${images[@]}"; do

  # Capture stderr in a variable
  err_output=$(./main.out ./testing/in/invalid/${image}.ppm -o ./testing/out/invalid/${image}.pgm 2>&1 >/dev/null)
  echo ""
  # Check if stderr is empty
  if [ -z "$err_output" ]; then

    echo -n "Failed - Test ${test_counter} - ${image}.ppm -o ${image}.pgm"

  else
    echo -n "Passed - Test ${test_counter} - ${image}.ppm -o ${image}.pgm"
    echo ""
    echo "Error Output: $err_output"
  fi
  ((test_counter++))

done

echo ""
