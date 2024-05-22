package cstring

import (
	cbase "github.com/jurgen-kluft/cbase/package"
	"github.com/jurgen-kluft/ccode/denv"
	ccore "github.com/jurgen-kluft/ccore/package"
	cunittest "github.com/jurgen-kluft/cunittest/package"
)

// GetPackage returns the package object of 'cstring'
func GetPackage() *denv.Package {
	// Dependencies
	unittestpkg := cunittest.GetPackage()
	ccorepkg := ccore.GetPackage()
	cbasepkg := cbase.GetPackage()

	// The main (cstring) package
	mainpkg := denv.NewPackage("cstring")
	mainpkg.AddPackage(unittestpkg)
	mainpkg.AddPackage(ccorepkg)
	mainpkg.AddPackage(cbasepkg)

	// 'cstring' library
	mainlib := denv.SetupDefaultCppLibProject("cstring", "github.com\\jurgen-kluft\\cstring")
	mainlib.Dependencies = append(mainlib.Dependencies, ccorepkg.GetMainLib())
	mainlib.Dependencies = append(mainlib.Dependencies, cbasepkg.GetMainLib())

	// 'cstring' unittest project
	maintest := denv.SetupDefaultCppTestProject("cstring_test", "github.com\\jurgen-kluft\\cstring")
	maintest.Dependencies = append(maintest.Dependencies, unittestpkg.GetMainLib())
	maintest.Dependencies = append(maintest.Dependencies, ccorepkg.GetMainLib())
	maintest.Dependencies = append(maintest.Dependencies, cbasepkg.GetMainLib())
	maintest.Dependencies = append(maintest.Dependencies, mainlib)

	mainpkg.AddMainLib(mainlib)
	mainpkg.AddUnittest(maintest)
	return mainpkg
}
