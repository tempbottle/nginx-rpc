# get from https://github.com/alibaba/tengine/archive/master.zip

rm -fr tengine-master
unzip tengine-master.zip
cd tengine-master

for f in `ls ../*.patch`
do
   echo ${f}
   patch -p 1 -i ${f}

done

