libclient="$NGX_OBJS${ngx_dirsep}libnginx.a"
libincs=" \$(CFLAGS) \$(CXXFLAGS) \$(ALL_INCS)"

for client in $NGX_CLIENT_BINS
do
    client_bin=`basename $client | cut -d . -f 1`
    client_bin="$NGX_OBJS${ngx_dirsep}$client_bin"
    
cat << END                                                    >> $NGX_MAKEFILE

$client_bin:	$libclient
	\$(LINK) ${ngx_long_start}${ngx_binout}$client_bin$ngx_long_cont$libincs$ngx_long_cont$client$ngx_long_cont$libclient$ngx_libs$ngx_link$NGX_EXTEND_LD_OPT$ngx_rcc
${ngx_long_end}
	test -f $NGX_PREFIX/sbin || mkdir -p $NGX_PREFIX/sbin
	cp -f $client_bin $NGX_PREFIX/sbin
END

done

