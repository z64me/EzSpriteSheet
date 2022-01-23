/*
 * viewer.js <z64.me>
 *
 * A reference implementation demonstrating how to use
 * EzSpriteSheet's JSON output with JavaScript.
 *
 */

/* global variables */
var displayGrid = true;
var view_width = 256;
var view_height = 256;
var animIndex = 0;
var mirrorX = 0;
var mirrorY = 0;
var spriteDB;

/* retrieve animation frame based on milliseconds elapsed */
function framenow(anim, ms)
{
	var cur = 0;
	var i = 0;
	
	ms %= anim.ms;
	
	while (i < anim.frames-1)
	{
		var frame = anim.frame[i];
		
		if (ms >= cur && ms < cur+frame.ms)
			return frame;
		
		cur += frame.ms;
		i++;
	}
	
	return anim.frame[i];
}

function button_prev()
{
	if (!animIndex)
		animIndex = spriteDB.animations;
	
	animIndex--;
}

function button_next()
{
	animIndex++;
	
	if (animIndex >= spriteDB.animations)
		animIndex = 0;
}

function button_mirrorX()
{
	mirrorX = !mirrorX;
}

function button_mirrorY()
{
	mirrorY = !mirrorY;
}

function button_grid()
{
	displayGrid = !displayGrid;
}

(function ()
{
			
	var spritesheetImage;
	var canvas;

	function mainLoop ()
	{
		window.requestAnimationFrame(mainLoop);
		
		/* clear canvas */
		var context = canvas.getContext("2d");
		context.clearRect(0, 0, view_width, view_height);	
		
		/* set up animation frame */
		var anim = spriteDB.animation[animIndex];
		var ms = Date.now();
		var frame = framenow(anim, ms);
		var center_x = view_width / 2;
		var center_y = view_height / 2;
		var x = center_x;
		var y = center_y;
		var w = frame.w;
		var h = frame.h;
		var wOff = w;
		var hOff = h;
		
		document.getElementById("animName").innerText = anim.name;
		
		/* swap mirror offset axes for rotated images */
		if (frame.rot)
		{
			wOff = h;
			hOff = w;
		}
		
		/* display the grid: a sprite's pivot point lives at its center */
		if (displayGrid)
		{
			context.globalAlpha = 0.25;
			context.fillRect(0, 0, center_x, center_y);
			context.fillRect(center_x, center_y, center_x, center_y);
			context.globalAlpha = 1.0;
		}
		
		context.save();
		
		/* x mirroring */
		if (mirrorX)
		{
			x += frame.ox - wOff;
			context.scale(-1, 1);
			w *= -1;
			x *= -1;
		} else
			x -= frame.ox;
		
		/* y mirroring */
		if (mirrorY)
		{
			y += frame.oy - hOff;
			context.scale(1, -1);
			h *= -1;
			y *= -1;
		} else
			y -= frame.oy;
		
		context.translate(x, y);
		
		/* sprite within sprite sheet is rotated */
		if (frame.rot)
		{
			if (mirrorX && mirrorY)
				context.translate(-frame.h, 0);
			else if (mirrorX)
				context.translate(0, frame.w);
			else if (mirrorY)
				context.translate(0, -hOff);
			else
				context.translate(frame.h, 0);
			
			context.rotate(1.5708); /* 90 degrees clockwise */
		}
		
		/* display */
		context.drawImage(
			spritesheetImage[frame.sheet],
			frame.x, /* cropping rectangle (from sprite sheet) */
			frame.y,
			frame.w,
			frame.h,
			0, /* positioning rectangle (onto canvas) */
			0,
			w,
			h
		);
		
		context.restore();
	}
	
	/* get canvas element */
	canvas = document.getElementById("viewer-canvas");
	canvas.width = view_width;
	canvas.height = view_height;

	/* load sprite database from json */
	$.ajax({
		url: "sprites.json",
		dataType: "json",
		async: false,
		success: function( data )
		{
			spriteDB = data;
			
			/* output sprite sheets onto web page */
			var items = [];
			for (var i = 0; i < data.sheets; i++)
				items.push( "<img class='sprite-sheet' src='" + data.sheet[i].source + "'>" );

			$( "<p/>", {
				html: items.join( "<br/>" )
			}).appendTo( "body" );
		}
	});
	
	/* load sprite sheet images */
	spritesheetImage = new Array();
	for (var i = 0; i < spriteDB.sheets; i++)
	{
		spritesheetImage[i] = new Image();
		spritesheetImage[i].src = spriteDB.sheet[i].source;
	}
	
	mainLoop();

} ());

