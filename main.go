package main

import (
	"github.com/docker/machine/libmachine/drivers/plugin"
	"github.com/SocketCluster/docker-machine-driver-baasil"
)

func main() {
	plugin.RegisterDriver(new(baasil.Driver))
}
