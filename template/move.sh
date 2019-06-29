#!/bin/sh

unlink sylar/bin/sylar
unlink bin/module/libproject_name.so
cp sylar/bin/sylar bin/project_name
cp lib/libproject_name.so bin/module/
