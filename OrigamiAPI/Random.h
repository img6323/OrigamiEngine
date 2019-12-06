/**
* @file Random.h
*/

#pragma once
#include "Prerequisites.h"

namespace OrigamiEngine {
	class Random
	{
	public:
		/**
		* @brief �����̃V�[�h�l��ύX����B
		* @param seed �V�[�h�l�B
		*/
		static void SetSeed(U32 seed);

		/**
		* @brief 0����2^32�܂ł̐����̗����𐶐�����B
		* @return  0����2^32�܂ł̐����̗����B
		*/
		static U32 GetU32();

		/**
		* @brief �����̗����𐶐�����B
		* @detail minimum����maximum�̊Ԃ��痐���𐶐�����B
		* @param minimum �����̍ŏ��l�B
		* @param maximum �����̍ő�l+1�B
		* @return minimum����maximum�̊Ԃ̗����B
		*/
		static I32 Range(I32 minimum, I32 maximum);


		/**
		* @brief �����̗����𐶐�����B
		* @detail minimum����maximum�̊Ԃ��痐���𐶐�����B
		* @param minimum �����̍ŏ��l�B
		* @param maximum �����̍ő�l�B
		* @return minimum����maximum�̊Ԃ̗����B
		*/
		static F32 Range(F32 minimum, F32 maximum);
	private:

		//! �����̐����Ɏg�p��������l�B
		static U32 x, y, z, w;
	};
}