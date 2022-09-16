YELLOW='\033[0;33m'
GREEN='\033[0;32m'
DEFAULT='\033[0m'
output_image="out.ppm"
reference_image="out2.ppm"
input_image="Images/tas.ppm"


function ppmcvt() {
    echo "${YELLOW}./ppmcvt $1 $2 -o ${output_image} $input_image${DEFAULT}"
    if ! ./ppmcvt $1 $2 -o ${output_image} $input_image; then
        return 1
    fi
    echo "${GREEN}Done${DEFAULT}"

    echo "${YELLOW}./ppmcvt.ref $1 $2 -o ${reference_image} $input_image${DEFAULT}"
    if ! ./ppmcvt.ref $1 $2 -o ${reference_image} $input_image; then
        return 1
    fi
    echo "${GREEN}Done${DEFAULT}"

    echo  "${YELLOW}diff ${output_image} ${reference_image} ${DEFAULT}"
    if diff ${output_image} ${reference_image}; then
        echo "${GREEN}✨ No differences found! ✨ ${DEFAULT}"
    fi
}
