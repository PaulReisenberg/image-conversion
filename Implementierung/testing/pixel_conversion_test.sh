#!/bin/bash

make

compare_files() {
  local file1="$1"
  local file2="$2"
  local max_diff="$3"

  # Check if both files exist
  if [[ ! -f "$file1" ]] || [[ ! -f "$file2" ]]; then
    echo "One or both files do not exist."
    return 1
  fi

  # Read files byte by byte
  while IFS= read -r -u3 -n1 byte1 && IFS= read -r -u4 -n1 byte2; do
    # Convert bytes to decimal values
    local ord1=$(LC_CTYPE=C printf '%d' "'$byte1")
    local ord2=$(LC_CTYPE=C printf '%d' "'$byte2")

    # Calculate the absolute difference
    local diff=$((ord1 - ord2))
    if ((diff < 0)); then diff=$((-diff)); fi

    # Check the difference
    if ((diff > max_diff)); then
      echo "Failed - Byte difference of $diff exceeds maximum allowed difference of $max_diff."
      return 1
    fi
  done 3<"$file1" 4<"$file2"

  echo "Passed - No byte differences exceed the maximum allowed difference of $max_diff."
  return 0
}

rm -f testing/out/valid/*pgm

# Array of test commands
declare -a tests=(
  "./main.out ./testing/in/valid/small.ppm --brightness=10 --contrast=10 -o testing/out/valid/small_con10_bri10_coeffs_standard.pgm"
  "./main.out ./testing/in/valid/pixel_edge_cases.ppm -o testing/out/valid/pixel_edge_cases_con0_bri0_coeffs_standard.pgm"
  "./main.out ./testing/in/valid/pixel_edge_cases.ppm --brightness=10 -o testing/out/valid/pixel_edge_cases_con0_bri10_coeffs_standard.pgm"
  "./main.out ./testing/in/valid/pixel_edge_cases.ppm --brightness=10 --contrast=15 -o testing/out/valid/pixel_edge_cases_con15_bri10_coeffs_standard.pgm"
  "./main.out ./testing/in/valid/pixel_edge_cases.ppm --coeffs=0.5,0.3,0.2 -o testing/out/valid/pixel_edge_cases_con0_bri0_coeffs_0-5_0-3_0-2.pgm"
  "./main.out ./testing/in/valid/pixel_edge_cases.ppm --coeffs=1.0,0.0,0.0 -o testing/out/valid/pixel_edge_cases_con0_bri0_coeffs_1_0_0.pgm"
)

# valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes
max_diff=1

# Test counter
test_counter=1

# Iterate over each test command
for test_cmd in "${tests[@]}"; do
  # Iterate over each version
  for version in {0..2}; do
    # Append the version option to the test command
    versioned_cmd="$test_cmd -V${version}"

    # Run the test command with version
    echo "Running Test ${test_counter}: $versioned_cmd"
    eval $versioned_cmd

    # Construct output and reference file names
    file=$(echo $test_cmd | grep -oP 'testing/out/valid/\K[^ ]*')

    output_file="testing/out/valid/${file}"
    reference_file="testing/reference/${file}"

    compare_files "${output_file}" "${reference_file}" ${max_diff}
    ((test_counter++))
    echo ""
  done
done
