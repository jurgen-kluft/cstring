package main

import (
	pkg "github.com/jurgen-kluft/xstring/package"
)

func main() {
	xcode.Init()
	xcode.Generate(pkg.GetPackage())
}
