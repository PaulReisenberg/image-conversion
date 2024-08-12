#!/bin/bash

make

# Array of invalid command calls
declare -a invalid_options=(
  "./main.out ./testing/in/valid/mandrill.ppm -o"                       # Missing output file name
  "./main.out ./testing/in/valid/mandrill.ppm -o not/exist.pgm"         # Non existent output folder
  "./main.out ./testing/in/valid/mandrill.ppm --unknownoption"          # Unknown option
  "./main.out ./testing/in/valid/mandrill.ppm --contrast=300"           # Contrast out of range
  "./main.out ./testing/in/valid/mandrill.ppm --coeffs 0,1"             # Missing two coefficients
  "./main.out ./testing/in/valid/mandrill.ppm --coeffs 1,2,3,4,5"       # Too many coefficients
  "./main.out"                                                          # No arguments
  "./main.out ./testing/in/valid/mandrill.ppm --brightness=300"         # Brightness out of range
  "./main.out ./testing/in/valid/mandrill.ppm --brightness=-256"        # Brightness out of range
  "./main.out ./testing/in/valid/mandrill.ppm --contrast=-256"          # Contrast out of range
  "./main.out ./testing/in/valid/mandrill.ppm --brightness=notanumber"  # Invalid brightness value
  "./main.out ./testing/in/valid/mandrill.ppm --contrast=notanumber"    # Invalid contrast value
  "./main.out ./testing/in/valid/mandrill.ppm --brightness="            # Missing brightness value
  "./main.out ./testing/in/valid/mandrill.ppm --contrast="              # Missing contrast value
  "./main.out ./testing/in/valid/mandrill.ppm --coeffs"                 # Missing coeffs values
  "./main.out ./testing/in/valid/mandrill.ppm --coeffs 0.3,0.6"         # Incomplete coeffs
  "./main.out ./testing/in/valid/mandrill.ppm --coeffs 0.3,0.6,0.1,0.2" # Excess coeffs
  "./main.out ./testing/in/valid/mandrill.ppm --coeffs a,b,c"           # Non-numeric coeffs
  "./main.out ./testing/in/valid/mandrill.ppm --coeffs 1,2"             # Missing one coefficient
  "./main.out ./testing/in/valid/mandrill.ppm -V"                       # Missing version number
  "./main.out ./testing/in/valid/mandrill.ppm -V abc"                   # Non-numeric version number
  "./main.out ./testing/in/valid/mandrill.ppm -B abc"                   # Non-numeric repetition count
  "./main.out --brightness=256 ./testing/in/valid/mandrill.ppm"         # Helligkeit außerhalb des Bereichs
  "./main.out --brightness=-256 ./testing/in/valid/mandrill.ppm"        # Helligkeit außerhalb des Bereichs
  "./main.out --contrast=256 ./testing/in/valid/mandrill.ppm"           # Kontrast außerhalb des Bereichs
  "./main.out --contrast=-256 ./testing/in/valid/mandrill.ppm"          # Kontrast außerhalb des Bereichs
  "./main.out --brightness=notanumber ./testing/in/valid/mandrill.ppm"  # Ungültiger Wert für Helligkeit
  "./main.out --contrast=notanumber ./testing/in/valid/mandrill.ppm"    # Ungültiger Wert für Kontrast
  "./main.out --coeffs 1,2 ./testing/in/valid/mandrill.ppm"             # Fehlender Koeffizient
  "./main.out --coeffs 1,2,3,4 ./testing/in/valid/mandrill.ppm"         # Zu viele Koeffizienten
  "./main.out --coeffs a,b,c ./testing/in/valid/mandrill.ppm"           # Nicht-numerische Koeffizienten
  "./main.out -V notanumber ./testing/in/valid/mandrill.ppm"            # Ungültiger Wert für Implementierungsnummer
  "./main.out -V -1 ./testing/in/valid/mandrill.ppm"                    # Implementierungsnummer außerhalb des gültigen Bereichs
  "./main.out -B notanumber ./testing/in/valid/mandrill.ppm"            # Ungültige Anzahl an Wiederholungen
  "./main.out -B -1 ./testing/in/valid/mandrill.ppm"                    # Negative Anzahl an Wiederholungen
  "./main.out -o ./testing/in/valid/mandrill.ppm"                       # Fehlender Dateiname für Ausgabedatei
  "./main.out --brightness= ./testing/in/valid/mandrill.ppm"            # Fehlender Wert für Helligkeit
  "./main.out --contrast= ./testing/in/valid/mandrill.ppm"              # Fehlender Wert für Kontrast
  "./main.out --coeffs= ./testing/in/valid/mandrill.ppm"                # Fehlende Koeffizienten
  "./main.out -V ./testing/in/valid/mandrill.ppm"                       # Fehlender Wert für Implementierungsnummer
  "./main.out --unknownoption ./testing/in/valid/mandrill.ppm"          # Unbekannte Option

)

test_counter=1

for cmd in "${invalid_options[@]}"; do
  err_output=$($cmd 2>&1 >/dev/null)

  echo ""
  # Check if stderr is not empty (indicating an error)
  if [ ! -z "$err_output" ]; then
    echo -n "Passed - Test ${test_counter} - Command: $cmd"
    # echo " Error Output: $err_output"
  else
    echo -n "Failed - Test ${test_counter} - Command: $cmd"
    echo " No error message for invalid input"
  fi
  ((test_counter++))
done
echo ""
