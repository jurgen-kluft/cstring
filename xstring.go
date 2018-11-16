package main

import (
	"github.com/jurgen-kluft/xcode"
	"github.com/jurgen-kluft/xstring/package"
)

func main() {
	xcode.Init()
	xcode.Generate(xstring.GetPackage())
}
