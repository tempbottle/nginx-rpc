#this is bash shell script

#target
libngx="$NGX_OBJS${ngx_dirsep}libnginx.a"


#1 rename main to _ngx_main in nginx.o , complie as ngx_main.o
ngx_main_objs="$NGX_OBJS${ngx_dirsep}src/core/ngx_main.o"
ngx_cc="\$(CC) $ngx_compile_opt \$(CFLAGS) $ngx_use_pch \$(ALL_INCS)"

cat << END                                                >> $NGX_MAKEFILE

$ngx_main_objs:	\$(CORE_DEPS)${ngx_cont}src/core/nginx.c
	$ngx_cc$ngx_tab-Dmain=_ngx_main$ngx_tab$ngx_objout$ngx_main_objs${ngx_tab}src/core/nginx.c$NGX_AUX

END


#2 link the libngx
libngx_objs=`echo ${ngx_objs} | sed -e "s|objs/src/core/nginx.o|$ngx_main_objs|" -e "s/ \\\\\\ /$ngx_long_regex_cont/g"`

cat << END                                                >> $NGX_MAKEFILE
$libngx:	$libngx_objs$ngx_spacer
	ar crv $libngx$ngx_long_cont$libngx_objs
${ngx_long_end}
	test -f $NGX_PREFIX/lib || mkdir -p $NGX_PREFIX/lib
	cp -f $libngx $NGX_PREFIX/lib
END

# 3 compile the client
libincs=" \$(CFLAGS) \$(CXXFLAGS) \$(ALL_INCS)"

for client in $NGX_CLIENT_BINS
do
    client_bin=`basename $client | cut -d . -f 1`
    client_bin="$NGX_OBJS${ngx_dirsep}$client_bin"

cat << END                                                    >> $NGX_MAKEFILE

$client_bin:	$client $libclient
	\$(LINK) ${ngx_long_start}${ngx_binout}$client_bin$ngx_long_cont$libincs$ngx_long_cont$client$ngx_long_cont$libngx$ngx_libs$ngx_link$NGX_EXTEND_LD_OPT$ngx_rcc
${ngx_long_end}
	test -f $NGX_PREFIX/sbin || mkdir -p $NGX_PREFIX/sbin
	cp -f $client_bin $NGX_PREFIX/sbin
END

done

