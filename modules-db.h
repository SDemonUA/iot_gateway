{
	.module_id = 1,
	.bundle = &IOT_MODULESDB_BUNDLE_OBJ(unet, generic__inputlinux),
	.module_name = "detector",
	.autoload = true,
	.autostart_detector = true,
	.item = NULL
},
{
	.module_id = 2,
	.bundle = &IOT_MODULESDB_BUNDLE_OBJ(unet, generic__inputlinux),
	.module_name = "input_drv",
	.autoload = true,
	.autostart_detector = false,
	.item = NULL
},
{
	.module_id = 3,
	.bundle = &IOT_MODULESDB_BUNDLE_OBJ(unet, generic__kbd),
	.module_name = "kbd_src",
	.autoload = false,
	.autostart_detector = false,
	.item = NULL
},
