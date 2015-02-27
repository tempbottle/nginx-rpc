ngx_client_srcs=`echo $CORE_SRCS $NGX_MISC_SRCS | sed -e "s|src/core/nginx.c||"`

if [ $HTTP = YES ]; then
    ngx_client_srcs="$ngx_client_srcs $HTTP_SRCS"
fi

if [ $MAIL = YES ]; then
    ngx_client_srcs="$ngx_client_srcs $MAIL_SRCS"
fi



ngx_client_srcs=`echo $ngx_client_srcs | sed -e "s/\//$ngx_regex_dirsep/g"`

for ngx_src in $NGX_CLIENT_SRCS
do
    ngx_obj="addon/`basename \`dirname $ngx_src\``"

    test -d $NGX_OBJS/$ngx_obj || mkdir -p $NGX_OBJS/$ngx_obj

    ngx_obj=`echo $ngx_obj/\`basename $ngx_src\` \
        | sed -e "s/\//$ngx_regex_dirsep/g"`
    ngx_client_srcs="$ngx_client_srcs $ngx_obj"
    
    if [ `expr match "$NGX_ADDON_SRCS" ".*$ngx_src"` -eq 0 ]; then
        
        ngx_obj=`echo $ngx_obj \
            | sed -e "s#^\(.*\.\)cpp\\$#$ngx_objs_dir\1$ngx_objext#g" \
                  -e "s#^\(.*\.\)cc\\$#$ngx_objs_dir\1$ngx_objext#g" \
                  -e "s#^\(.*\.\)c\\$#$ngx_objs_dir\1$ngx_objext#g" \
                  -e "s#^\(.*\.\)S\\$#$ngx_objs_dir\1$ngx_objext#g"`
        
        ngx_src=`echo $ngx_src | sed -e "s/\//$ngx_regex_dirsep/g"`
        
        ngx_cc="\$(CC) $ngx_compile_opt \$(CFLAGS) $ngx_use_pch \$(ALL_INCS)"
        
        #if the ngx_src is c++ file, add CXXFLAGS to ngx_cc.
        if [ "${ngx_src##*.}" = "cc" ] || [ "${ngx_src##*.}" = "cpp" ];then
            ngx_cc="\$(CC) $ngx_compile_opt \$(CFLAGS) \$(CXXFLAGS) $ngx_use_pch \$(ALL_INCS)"
        fi

        cat << END                                            >> $NGX_MAKEFILE

$ngx_obj:	\$(ADDON_DEPS)$ngx_cont$ngx_src
	$ngx_cc$ngx_tab$ngx_objout$ngx_obj$ngx_tab$ngx_src$NGX_AUX

END
    fi
    
done

ngx_client_objs=`echo $ngx_client_srcs \
    | sed -e "s#\([^ ]*\.\)cpp#$NGX_OBJS\/\1$ngx_objext#g" \
          -e "s#\([^ ]*\.\)cc#$NGX_OBJS\/\1$ngx_objext#g" \
          -e "s#\([^ ]*\.\)c#$NGX_OBJS\/\1$ngx_objext#g" \
          -e "s#\([^ ]*\.\)S#$NGX_OBJS\/\1$ngx_objext#g"`

ngx_lib_objs=`echo $ngx_client_objs $ngx_modules_obj \
    | sed -e "s/  *\([^ ][^ ]*\)/$ngx_long_regex_cont\1/g" \
          -e "s/\//$ngx_regex_dirsep/g"`

if test -n "$ngx_lib_objs"; then

cat << END                                                    >> $NGX_MAKEFILE
$NGX_OBJS${ngx_dirsep}libnginx.a${ngx_binext}:	$ngx_lib_objs$ngx_spacer
	ar crv $NGX_OBJS${ngx_dirsep}libnginx.a$ngx_long_cont${ngx_lib_objs}
${ngx_long_end}
	test -f $NGX_PREFIX/lib || mkdir -p $NGX_PREFIX/lib
	cp -f $NGX_OBJS${ngx_dirsep}libnginx.a $NGX_PREFIX/lib
END

fi
