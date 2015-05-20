strings logs/error.log |grep ngx_slab.c |grep "src/core/ngx_slab.c:395"|awk '{print $7}' |sort > alloc.txt
strings logs/error.log |grep ngx_slab.c |grep "src/core/ngx_slab.c:420" | awk '{print $7}' |sort > free.txt
diff -a -y -d  --suppress-common-lines alloc.txt  free.txt
rm  alloc.txt free.txt

