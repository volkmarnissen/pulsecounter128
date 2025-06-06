# Add external components:
# PCF8574


#  
addComponent()
{
    COMPONENT=$1
    ADDITIONAL=$2
    mkdir -p $COMPONENT
    {
        echo  .eil.yml
        echo CMakeLists.txt
        if [ "$ADDITIONAL" ! ""]
        then 
            echo $ADDITIONAL
        fi
        echo LICENSE
        echo component.mk
        echo $COMPONENT.c
        echo $COMPONENT.h
    }|while read f 
    do
        curl -o components/$COMPONENT/$f https://raw.githubusercontent.com/UncleRus/esp-idf-lib/refs/heads/master/components/$COMPONENT/$f
    done
}
mkdir -p components
addComponent esp_idf_lib_helpers ""
addComponent i2cdev "Kconfig"
addComponent pcf8574 ""

