--
-- Caracas awesome configuration
--

-- Standard awesome library
require("awful")
require("awful.autofocus")
require("awful.rules")
-- Theme handling library
require("beautiful")
-- Notification library
require("naughty")

--local radical = require("radical")

-- Load default theme
beautiful.init(awful.util.getdir("config") .. "/theme.lua")

-- Only one tag needed for this setup
tags = awful.tag({1}, 1, awful.layout.suit.max)

-- Click to focus and raise
clientbuttons = awful.util.table.join(
    awful.button({ }, 1, function (c) client.focus = c; c:raise() end)
)

-- Client rules
awful.rules.rules = {
    -- All clients will match this rule.
    {
        rule = { },
        properties = {
            border_width = 0,
            focus = true,
            buttons = clientbuttons
        }
    },
    {
        rule = { class = "xvkbd" },
        properties = {
            floating = true
        }
    }
}

-- Signal function to execute when a new client appears.
client.add_signal("manage", function (c, startup)
    if not startup then
        -- Put windows in a smart way, only if they does not set an initial position.
        if not c.size_hints.user_position and not c.size_hints.program_position then
            awful.placement.centered(c)
        end
    end
end)
