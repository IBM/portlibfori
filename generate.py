import json
import sys

from os.path import dirname
from glob import glob

svr4_soname = True

print("""
CC=gcc
CFLAGS=-D__PASE__ -fPIC
LDFLAGS=
PREFIX=/usr/local
DESTDIR=

all: build-all

install: install-all
""".lstrip())

all_targets = []
all_install = []

for cfg in sorted(glob('*/build.json')):
    cfg_dir = dirname(cfg)

    try:
        info = json.load(open(cfg))
        if not isinstance(info, dict):
            continue
    except json.decoder.JSONDecodeError as e:
        print(e, file=sys.stderr)
        continue

    install_libs = []

    for target in info.get('targets', []):
        if target['type'] == 'library':
            name = target['name']

            all_targets.append(f"{cfg_dir}/{name}.target")

            sover = target.get('version', 1)
            soname = f"{name}.so.{sover}"

            all_install.append(f"install-{cfg_dir}-{name}")
            install_libs.append(f"{cfg_dir}/{name}.so")
            install_libs.append(f"{cfg_dir}/{soname}")

            objs = " ".join([ f"{cfg_dir}/{_}" for _ in target['objects'] ])

            out = f"""
{cfg_dir}/{name}.o: {objs}
	$(CC) -shared $(CFLAGS) $(LDFLAGS) -Wl,-bE:{cfg_dir}/{name}.exp -o $@ $^

{cfg_dir}/%.o: {cfg_dir}/%.c
	$(CC) -c $(CFLAGS) -I{cfg_dir} -o $@ $^

{cfg_dir}/{name}.imp: {cfg_dir}/{name}.exp
	( \\
	echo '#! {soname}(shr_64.o)'; \\
	echo "# 64"; \\
	cat {cfg_dir}/{name}.exp; \\
	) > {cfg_dir}/{name}.imp

{cfg_dir}/{soname}: {cfg_dir}/{name}.o {cfg_dir}/{name}.imp
	export OBJECT_MODE=32_64
	mkdir -p {cfg_dir}/{name}.tmp
	cp {cfg_dir}/{name}.o {cfg_dir}/{name}.tmp/shr_64.o
	cp {cfg_dir}/{name}.imp {cfg_dir}/{name}.tmp/shr_64.imp
	cd {cfg_dir}/{name}.tmp
	strip -e {cfg_dir}/{name}.tmp/shr_64.o 2> /dev/null || :
	ar -X64 crlo $@ {cfg_dir}/{name}.tmp/*
	rm -r {cfg_dir}/{name}.tmp

{cfg_dir}/{name}.so: {cfg_dir}/{soname}
	ln -sf {soname} {cfg_dir}/{name}.so

{cfg_dir}/{name}.target: {cfg_dir}/{name}.so
"""
            print(out.lstrip())

    print(f'install-{cfg_dir}-{name}: ' + " ".join(install_libs))

    if install_libs:
        print(f"	mkdir -p $(DESTDIR)$(PREFIX)/lib")
        
    for lib in install_libs:
        print(f"	/QOpenSys/usr/bin/cp -h {lib} $(DESTDIR)$(PREFIX)/lib")
    
    if 'install' in info:
        install = info['install']

        if 'includes' in install:
            print(f"	mkdir -p $(DESTDIR)$(PREFIX)/include")

            dirs = set()
            for item in install['includes']:
                dstfile = item if isinstance(item, str) else item[1]

                directory = dirname(dstfile)
                if not directory or directory in dirs:
                    continue

                dirs.add(directory)
                print(f"	mkdir -p $(DESTDIR)$(PREFIX)/include/{directory}")

            for item in install['includes']:
                if isinstance(item, str):
                    srcfile = dstfile = item
                else:
                    srcfile, dstfile = item
                print(f"	cp {cfg_dir}/{srcfile} $(DESTDIR)$(PREFIX)/include/{dstfile}")
    print()
        
print(f"""
build-all: {" ".join(all_targets)}

install-all: {" ".join(all_install)}

.PHONY: clean

clean:
	rm -f */*.a */*.o */*.so */*.so.* */*.imp
""".strip())
