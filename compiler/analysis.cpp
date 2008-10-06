/*
	Aseba - an event-based framework for distributed robot control
	Copyright (C) 2006 - 2008:
		Stephane Magnenat <stephane at magnenat dot net>
		(http://stephane.magnenat.net)
		and other contributors, see authors.txt for details
		Mobots group, Laboratory of Robotics Systems, EPFL, Lausanne
	
	This program is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	any other version as decided by the two original authors
	Stephane Magnenat and Valentin Longchamp.
	
	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.
	
	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "compiler.h"
#include "../common/consts.h"
#include <cassert>
#include <iostream>

namespace Aseba
{
	/** \addtogroup compiler */
	/*@{*/
	
	//! Verify that no call path can create a stack overflow
	bool Compiler::verifyStackCalls(PreLinkBytecode& preLinkBytecode)
	{
		// check stack for events
		for (PreLinkBytecode::EventsBytecode::iterator it = preLinkBytecode.events.begin(); it != preLinkBytecode.events.end(); ++it)
		{
			if (it->second.maxStackDepth > targetDescription->stackSize)
				return false;
			
			const BytecodeVector& bytecode = it->second;
			for (size_t pc = 0; pc < bytecode.size();)
			{
				switch (bytecode[pc] >> 12)
				{
					case ASEBA_BYTECODE_SUB_CALL:
					{
						unsigned id = bytecode[pc] & 0x0fff;
						PreLinkBytecode::SubroutinesBytecode::iterator destIt = preLinkBytecode.subroutines.find(id);
						assert(destIt != preLinkBytecode.subroutines.end());
						destIt->second.callDepth = 1;
						pc += 1;
					}
					break;
					
					case ASEBA_BYTECODE_LARGE_IMMEDIATE:
					case ASEBA_BYTECODE_LOAD_INDIRECT:
					case ASEBA_BYTECODE_STORE_INDIRECT:
					case ASEBA_BYTECODE_CONDITIONAL_BRANCH:
						pc += 2;
					break;
					
					case ASEBA_BYTECODE_EMIT:
						pc += 3;
					break;
					
					default:
						pc += 1;
					break;
				}
			}
		}
		
		// check stack for subroutines
		bool wasActivity;
		do
		{
			wasActivity = false;
			for (PreLinkBytecode::SubroutinesBytecode::iterator it = preLinkBytecode.subroutines.begin(); it != preLinkBytecode.subroutines.end(); ++it)
			{
				unsigned myDepth = it->second.callDepth;
				if (myDepth + it->second.maxStackDepth > targetDescription->stackSize)
				{
					return false;
				}
				
				const BytecodeVector& bytecode = it->second;
				for (size_t pc = 0; pc < bytecode.size();)
				{
					switch (bytecode[pc] >> 12)
					{
						case ASEBA_BYTECODE_SUB_CALL:
						{
							unsigned id = bytecode[pc] & 0x0fff;
							PreLinkBytecode::SubroutinesBytecode::iterator destIt = preLinkBytecode.subroutines.find(id);
							assert(destIt != preLinkBytecode.subroutines.end());
							if (myDepth + 1 > destIt->second.callDepth)
							{
								wasActivity = true;
								destIt->second.callDepth = myDepth + 1;
							}
							pc += 1;
						}
						break;
						
						case ASEBA_BYTECODE_LARGE_IMMEDIATE:
						case ASEBA_BYTECODE_LOAD_INDIRECT:
						case ASEBA_BYTECODE_STORE_INDIRECT:
						case ASEBA_BYTECODE_CONDITIONAL_BRANCH:
							pc += 2;
						break;
						
						case ASEBA_BYTECODE_EMIT:
							pc += 3;
						break;
						
						default:
							pc += 1;
						break;
					}
				}
			}
		}
		while (wasActivity);
		
		return true;
	}
	
	/*@}*/
	
} // Aseba
