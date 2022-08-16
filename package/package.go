package xstring

import (
	"github.com/jurgen-kluft/ccode/denv"
	"github.com/jurgen-kluft/xbase/package"
	"github.com/jurgen-kluft/xentry/package"
)

// GetPackage returns the package object of 'xstring'
func GetPackage() *denv.Package {
	// Dependencies
	unittestpkg := xunittest.GetPackage()
	entrypkg := xentry.GetPackage()
	xbasepkg := xbase.GetPackage()

	// The main (xstring) package
	mainpkg := denv.NewPackage("xstring")
	mainpkg.AddPackage(unittestpkg)
	mainpkg.AddPackage(entrypkg)
	mainpkg.AddPackage(xbasepkg)

	// 'xstring' library
	mainlib := denv.SetupDefaultCppLibProject("xstring", "github.com\\jurgen-kluft\\xstring")
	mainlib.Dependencies = append(mainlib.Dependencies, xbasepkg.GetMainLib())

	// 'xstring' unittest project
	maintest := denv.SetupDefaultCppTestProject("xstring_test", "github.com\\jurgen-kluft\\xstring")
	maintest.Dependencies = append(maintest.Dependencies, unittestpkg.GetMainLib())
	maintest.Dependencies = append(maintest.Dependencies, entrypkg.GetMainLib())
	maintest.Dependencies = append(maintest.Dependencies, xbasepkg.GetMainLib())
	maintest.Dependencies = append(maintest.Dependencies, mainlib)

	mainpkg.AddMainLib(mainlib)
	mainpkg.AddUnittest(maintest)
	return mainpkg
}
