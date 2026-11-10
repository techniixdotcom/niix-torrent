-- The theme now follows the operating system by default; "dark" and "light"
-- can still be forced from Preferences > General.
UPDATE setting SET value = '"system"', default_value = '"system"' WHERE key = 'theme_id';
