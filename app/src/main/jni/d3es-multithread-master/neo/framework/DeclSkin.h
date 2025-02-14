/*
===========================================================================

Doom 3 GPL Source Code
Copyright (C) 1999-2011 id Software LLC, a ZeniMax Media company.

This file is part of the Doom 3 GPL Source Code (?Doom 3 Source Code?).

Doom 3 Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Doom 3 Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Doom 3 Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Doom 3 Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the Doom 3 Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/

#ifndef __DECLSKIN_H__
#define __DECLSKIN_H__

/*
===============================================================================

	idDeclSkin

===============================================================================
*/

typedef struct {
	const idMaterial 		*from;			// 0 == any unmatched shader
	const idMaterial 		*to;
} skinMapping_t;

class idDeclSkin : public idDecl
{
	public:
		virtual size_t			Size(void) const;
		virtual bool			SetDefaultText(void);
		virtual const char 	*DefaultDefinition(void) const;
		virtual bool			Parse(const char *text, const int textLength);
		virtual void			FreeData(void);

#ifdef _HUMANHEAD
		virtual //HUMANHEAD: aob - needed so this is added to vtable and callable from DLL
#endif
		const idMaterial 		*RemapShaderBySkin(const idMaterial *shader) const;

		// model associations are just for the preview dialog in the editor
		const int				GetNumModelAssociations() const;
		const char 			*GetAssociatedModel(int index) const;

	private:
		idList<skinMapping_t>	mappings;
		idStrList				associatedModels;
};

#endif /* !__DECLSKIN_H__ */
