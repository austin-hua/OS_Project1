# repeatedly test the scheduler
# If the scheduler ends abnormally, write the error message into error.txt.

while true; do
    for file in OS_PJ1_Test/*.txt; do
        msg=$(./main <$file 2>&1)
        if [ $? -ne 0 ] ; then
            echo "---Error encountered while testing $file" >>error.txt
            echo "$msg" >>error.txt
            echo >> error.txt
        fi
    done
done

