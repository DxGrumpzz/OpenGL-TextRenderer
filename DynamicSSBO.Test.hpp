#pragma once

#include "DynamicSSBO.hpp"


inline void TestDynamicSSBO()
{
    // Uint32 - vec4 padding
    {
        RawLayout rawLayout;

        rawLayout.Add<ScalarElement, DataType::UInt32>("Uint_off_0");
        rawLayout.Add<ScalarElement, DataType::Vec4f>("Vec4_off_16");


        SSBOLayout layout = SSBOLayout (rawLayout);

        if(layout.Get<ScalarElement>("Uint_off_0")->GetOffset() != 0)
            __debugbreak();
        if(layout.Get<ScalarElement>("Vec4_off_16")->GetOffset() != 16)
            __debugbreak();
    };


    // Contigious elements
    {
        RawLayout rawLayout;

        rawLayout.Add<ScalarElement, DataType::UInt32>("Uint_off_0");
        rawLayout.Add<ScalarElement, DataType::UInt32>("Uint_off_4");
        rawLayout.Add<ScalarElement, DataType::UInt32>("Uint_off_8");
        rawLayout.Add<ScalarElement, DataType::UInt32>("Uint_off_12");
        rawLayout.Add<ScalarElement, DataType::UInt32>("Uint_off_16");


        SSBOLayout layout = SSBOLayout (rawLayout);

        if(layout.Get<ScalarElement>("Uint_off_0")->GetOffset() != 0)
            __debugbreak();
        if(layout.Get<ScalarElement>("Uint_off_4")->GetOffset() != 4)
            __debugbreak();
        if(layout.Get<ScalarElement>("Uint_off_8")->GetOffset() != 8)
            __debugbreak();
        if(layout.Get<ScalarElement>("Uint_off_12")->GetOffset() != 12)
            __debugbreak();
        if(layout.Get<ScalarElement>("Uint_off_16")->GetOffset() != 16)
            __debugbreak();
    };


    {
        RawLayout rawLayout;

        rawLayout.Add<ScalarElement, DataType::Vec2f>("Vec2_off_0");
        rawLayout.Add<ScalarElement, DataType::UInt32>("Uint_off_8");

        SSBOLayout layout = SSBOLayout (rawLayout);

        if(layout.Get<ScalarElement>("Vec2_off_0")->GetOffset() != 0)
            __debugbreak();
        if(layout.Get<ScalarElement>("Uint_off_8")->GetOffset() != 8)
            __debugbreak();
    };


    {
        RawLayout rawLayout;

        rawLayout.Add<ScalarElement, DataType::Vec2f>("Vec2_off_0");
        rawLayout.Add<ScalarElement, DataType::Vec2f>("Vec2_off_8");

        SSBOLayout layout = SSBOLayout (rawLayout);

        if(layout.Get<ScalarElement>("Vec2_off_0")->GetOffset() != 0)
            __debugbreak();
        if(layout.Get<ScalarElement>("Vec2_off_8")->GetOffset() != 8)
            __debugbreak();
    };


    // Array 
    {
        RawLayout rawLayout;

        auto rawArrayElement = rawLayout.Add<ArrayElement, DataType::Array>("Uint_off_0");

        rawArrayElement->SetArray(DataType::UInt32, 4);

        SSBOLayout layout = SSBOLayout(rawLayout);

        auto arrayElement = layout.Get<ArrayElement>("Uint_off_0");

        auto s = layout.Get<ArrayElement>("Uint_off_0");

        if(layout.Get<ArrayElement>("Uint_off_0")->GetOffset() != 0)
            __debugbreak();

        if(arrayElement->GetAtIndex<ScalarElement>(0)->GetOffset() != 0)
            __debugbreak();
        if(arrayElement->GetAtIndex<ScalarElement>(1)->GetOffset() != 4)
            __debugbreak();
        if(arrayElement->GetAtIndex<ScalarElement>(2)->GetOffset() != 8)
            __debugbreak();
        if(arrayElement->GetAtIndex<ScalarElement>(3)->GetOffset() != 12)
            __debugbreak();

    };

    // Array 2
    {
        RawLayout rawLayout;

        auto rawArrayElement = rawLayout.Add<ArrayElement, DataType::Array>("Uint_off_0");

        rawArrayElement->SetArray(DataType::UInt32, 3);

        rawLayout.Add<ScalarElement, DataType::UInt32>("Uint_off_12");

        SSBOLayout layout = SSBOLayout(rawLayout);

        auto arrayElement = layout.Get<ArrayElement>("Uint_off_0");

        if(layout.Get<ArrayElement>("Uint_off_0")->GetOffset() != 0)
            __debugbreak();

        if(arrayElement->GetAtIndex<ScalarElement>(0)->GetOffset() != 0)
            __debugbreak();
        if(arrayElement->GetAtIndex<ScalarElement>(1)->GetOffset() != 4)
            __debugbreak();
        if(arrayElement->GetAtIndex<ScalarElement>(2)->GetOffset() != 8)
            __debugbreak();

        if(layout.Get<ScalarElement>("Uint_off_12")->GetOffset() != 12)
            __debugbreak();
    };


    // Array with padding
    {
        RawLayout rawLayout;

        rawLayout.Add<ScalarElement, DataType::UInt32>("Uint_off_0");

        auto rawArrayElement = rawLayout.Add<ArrayElement, DataType::Array>("Array_off_16");

        rawArrayElement->SetArray(DataType::Vec4f, 3);

        SSBOLayout layout = SSBOLayout(rawLayout);

        auto arrayElement = layout.Get<ArrayElement>("Array_off_16");


        if(layout.Get<ScalarElement>("Uint_off_0")->GetOffset() != 0)
            __debugbreak();

        if(layout.Get<ArrayElement>("Array_off_16")->GetOffset() != 16)
            __debugbreak();

        if(arrayElement->GetAtIndex<ScalarElement>(0)->GetOffset() != 16)
            __debugbreak();
        if(arrayElement->GetAtIndex<ScalarElement>(1)->GetOffset() != 32)
            __debugbreak();
        if(arrayElement->GetAtIndex<ScalarElement>(2)->GetOffset() != 48)
            __debugbreak();

    };


    // Struct
    {
        RawLayout structLayout;

        structLayout.Add<ScalarElement, DataType::UInt32>("Uint_off_0");

        auto rawStructElement = structLayout.Add<StructElement, DataType::Struct>("Test_off_16");

        rawStructElement->Add<ScalarElement, DataType::UInt32>("Test_Uint_off_16");
        rawStructElement->Add<ScalarElement, DataType::Vec4f>("Test_Vec4_off_32");

        structLayout.Add<ScalarElement, DataType::UInt32>("Uint_off_48");

        SSBOLayout layout = SSBOLayout(structLayout);

        auto structElement = layout.Get<StructElement>("Test_off_16");


        if(layout.Get<ScalarElement>("Uint_off_0")->GetOffset() != 0)
            __debugbreak();

        if(layout.Get<StructElement>("Test_off_16")->GetOffset() != 16)
            __debugbreak();

        if(structElement->Get<ScalarElement>("Test_Uint_off_16")->GetOffset() != 16)
            __debugbreak();

        if(structElement->Get<ScalarElement>("Test_Vec4_off_32")->GetOffset() != 32)
            __debugbreak();

        if(layout.Get<ScalarElement>("Uint_off_48")->GetOffset() != 48)
            __debugbreak();
    };


    // Multiple structs
    {
        RawLayout structLayout;

        structLayout.Add<ScalarElement, DataType::UInt32>("Uint_off_0");

        auto rawStructElement = structLayout.Add<StructElement, DataType::Struct>("Test_off_16");

        rawStructElement->Add<ScalarElement, DataType::UInt32>("Test_Uint_off_16");
        rawStructElement->Add<ScalarElement, DataType::Vec4f>("Test_Vec4_off_32");


        auto rawStructElement2 = structLayout.Add<StructElement, DataType::Struct>("Test2_off_48");

        rawStructElement2->Add<ScalarElement, DataType::UInt32>("Test2_Uint_off_48");
        rawStructElement2->Add<ScalarElement, DataType::Mat4f>("Test2_Mat4_off_64");


        SSBOLayout layout = SSBOLayout(structLayout);

        auto structElement = layout.Get<StructElement>("Test_off_16");
        auto structElement2 = layout.Get<StructElement>("Test2_off_48");


        if(layout.Get<ScalarElement>("Uint_off_0")->GetOffset() != 0)
            __debugbreak();

        if(layout.Get<StructElement>("Test_off_16")->GetOffset() != 16)
            __debugbreak();

        if(structElement->Get<ScalarElement>("Test_Uint_off_16")->GetOffset() != 16)
            __debugbreak();

        if(structElement->Get<ScalarElement>("Test_Vec4_off_32")->GetOffset() != 32)
            __debugbreak();

        if(layout.Get<StructElement>("Test2_off_48")->GetOffset() != 48)
            __debugbreak();

        if(structElement2->Get<ScalarElement>("Test2_Uint_off_48")->GetOffset() != 48)
            __debugbreak();

        if(structElement2->Get<ScalarElement>("Test2_Mat4_off_64")->GetOffset() != 64)
            __debugbreak();

    };


    // Array of structs
    {
        RawLayout structArrayLayout;

        auto rawArrayElement = structArrayLayout.Add<ArrayElement, DataType::Array>("Test_off_0");

        auto arrayType = rawArrayElement->SetCustomArrayType(5);

        arrayType->Add<ScalarElement, DataType::UInt32>("Test_Uint_off_0");
        arrayType->Add<ScalarElement, DataType::Vec4f>("Test_Vec4_off_16");


        structArrayLayout.Add<ScalarElement, DataType::UInt32>("Uint_off_160");


        SSBOLayout layout = SSBOLayout(structArrayLayout);

        auto structArray = layout.Get<ArrayElement>("Test_off_0");

        if(structArray->GetOffset() != 0)
            __debugbreak();

        for(std::size_t i = 0; i < 5; ++i)
        {
            if(structArray->GetAtIndex<StructElement>(i)->GetOffset() != i * 32)
                __debugbreak();


            if(structArray->GetAtIndex<StructElement>(i)->Get<ScalarElement>("Test_Uint_off_0")->GetOffset() != i * 32)
                __debugbreak();

            if(structArray->GetAtIndex<StructElement>(i)->Get<ScalarElement>("Test_Vec4_off_16")->GetOffset() != (i * 32) + 16)
                __debugbreak();
        };

        if(layout.Get<ScalarElement>("Uint_off_160")->GetOffset() != 160)
            __debugbreak();

    };


    // Array of structs with padding
    {
        RawLayout structArrayLayout;

        structArrayLayout.Add<ScalarElement, DataType::UInt32>("Uint_off_0");

        auto rawArrayElement = structArrayLayout.Add<ArrayElement, DataType::Array>("Test_off_16");

        auto arrayType = rawArrayElement->SetCustomArrayType(5);

        arrayType->Add<ScalarElement, DataType::UInt32>("Test_Uint_off_16");
        arrayType->Add<ScalarElement, DataType::Vec4f>("Test_Vec4_off_32");

        structArrayLayout.Add<ScalarElement, DataType::UInt32>("Uint_off_176");


        SSBOLayout layout = SSBOLayout(structArrayLayout);

        auto structArray = layout.Get<ArrayElement>("Test_off_16");

        if(structArray->GetOffset() != 16)
            __debugbreak();

        for(std::size_t i = 0; i < 5; ++i)
        {
            if(structArray->GetAtIndex<StructElement>(i)->GetOffset() != (i * 32) + 16)
                __debugbreak();


            if(structArray->GetAtIndex<StructElement>(i)->Get<ScalarElement>("Test_Uint_off_16")->GetOffset() != (i * 32) + 16)
                __debugbreak();

            if(structArray->GetAtIndex<StructElement>(i)->Get<ScalarElement>("Test_Vec4_off_32")->GetOffset() != ((i * 32) + 16) + 16)
                __debugbreak();
        };

        if(layout.Get<ScalarElement>("Uint_off_176")->GetOffset() != 176)
            __debugbreak();

    };


    {
        RawLayout structArrayLayout;

        auto rawArrayElement = structArrayLayout.Add<ArrayElement, DataType::Array>("Test_off_0");

        auto arrayType = rawArrayElement->SetCustomArrayType(3);

        arrayType->Add<ScalarElement, DataType::UInt32>("Test_Uint");
        arrayType->Add<ScalarElement, DataType::Vec2f>("Test_Vec2");


        SSBOLayout layout = SSBOLayout(structArrayLayout);

        auto structArray = layout.Get<ArrayElement>("Test_off_0");

        if(structArray->GetOffset() != 0)
            __debugbreak();

        for(std::size_t i = 0; i < 3; ++i)
        {
            if(structArray->GetAtIndex<StructElement>(i)->GetOffset() != (i * 16))
                __debugbreak();


            if(structArray->GetAtIndex<StructElement>(i)->Get<ScalarElement>("Test_Uint")->GetOffset() != (i * 16))
                __debugbreak();

            if(structArray->GetAtIndex<StructElement>(i)->Get<ScalarElement>("Test_Vec2")->GetOffset() != ((i * 16) + 8))
                __debugbreak();
        };

        int _ = 0;
    };


    {
        RawLayout structArrayLayout;

        auto rawArrayElement = structArrayLayout.Add<ArrayElement, DataType::Array>("Test_Array");

        auto arrayType = rawArrayElement->SetCustomArrayType(3);

        arrayType->Add<ScalarElement, DataType::UInt32>("a");
        arrayType->Add<ScalarElement, DataType::UInt32>("b");
        arrayType->Add<ScalarElement, DataType::UInt32>("c");


        SSBOLayout layout = SSBOLayout(structArrayLayout);

        auto structArray = layout.Get<ArrayElement>("Test_Array");

        if(structArray->GetOffset() != 0)
            __debugbreak();


        for(std::size_t i = 0; i < 3; ++i)
        {
            if(structArray->GetAtIndex<StructElement>(i)->GetOffset() != (i * 16))
                __debugbreak();


            if(structArray->GetAtIndex<StructElement>(i)->Get<ScalarElement>("a")->GetOffset() != (i * 16))
                __debugbreak();

            if(structArray->GetAtIndex<StructElement>(i)->Get<ScalarElement>("b")->GetOffset() != ((i * 16) + 4))
                __debugbreak();

            if(structArray->GetAtIndex<StructElement>(i)->Get<ScalarElement>("c")->GetOffset() != ((i * 16) + 8))
                __debugbreak();
        };

    };

    // Struct test
    {
        RawLayout rawLayout;

        rawLayout.Add<ScalarElement, DataType::UInt32>("Uint_off_0");

        auto rawStructElement = rawLayout.Add<StructElement, DataType::Struct>("Struct_off_4");

        rawStructElement->Add < ScalarElement, DataType::UInt32>("Struct.Uint_off_4");

        SSBOLayout layout = SSBOLayout(rawLayout);

        if(layout.Get<ScalarElement>("Uint_off_0")->GetOffset() != 0)
            __debugbreak();

        if(layout.Get<StructElement>("Struct_off_4")->GetOffset() != 4)
            __debugbreak();


        if(layout.Get<StructElement>("Struct_off_4")->Get<ScalarElement>("Struct.Uint_off_4")->GetOffset() != 4)
            __debugbreak();

    };
};