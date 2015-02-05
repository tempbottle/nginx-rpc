a="cc"
b="c"
if [ $a = c ];then
   echo "1"
fi 

if [ $b = c ];then
   echo "2"
fi 

if [[ $a="c" ]];then
   echo "3"
fi 

if [[ $b="c" ]];then
   echo "4"
fi


if [ $a=="c" ];then
   echo "5"
fi 

if [ $b=="c" ];then
   echo "6"
fi 

if [[ $a=="c" ]];then
   echo "7"
fi 

if [[ $b=="c" ]];then
   echo "8"
fi  
