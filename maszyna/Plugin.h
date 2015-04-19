#ifndef PLUGIN_H
#define PLUGIN_H

enum PluginType
{
	RENDERER,
	VESSEL,
	STATIC,
	DYNAMIC
};

struct PluginDesc
{
	char *name;
	char *description;
	PluginType ptype;
};

#endif