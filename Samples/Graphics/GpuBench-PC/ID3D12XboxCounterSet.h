//--------------------------------------------------------------------------------------
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

extern "C"
{

	//
	// ID3D12XboxCounterSet
	//

	struct D3D12CounterSet_Function_Table
	{
		D3D12DeviceChild_Function_Table DeviceChildFunction;

		SIZE_T (D3DAPI* GetCounterDataSizeX)(
			_Inout_ ID3D12XboxCounterSet* pCounterSet);

		HRESULT (D3DAPI* RetrieveCounterDataX)(
			_Inout_ ID3D12XboxCounterSet* pCounterSet, 
			_In_ ID3D12Resource* pSourceBuffer,
			_In_ UINT64 AlignedSourceBufferOffset, 
			_Out_ D3D12XBOX_COUNTER_DATA* pCounterData);
	};

#if DECLARE_UMD_EXPORTS_D3D12X

	ULONG D3DAPI D3d12CounterSet_Release(
		_Inout_ IGraphicsUnknown* pCounterSet);

	SIZE_T D3DAPI D3D12CounterSet_GetCounterDataSizeX(
		_Inout_ ID3D12XboxCounterSet* pCounterSet);

	HRESULT D3DAPI D3D12CounterSet_RetrieveCounterDataX(
		_Inout_ ID3D12XboxCounterSet* pCounterSet, 
		_In_ ID3D12Resource* pSourceBuffer,
		_In_ UINT64 AlignedSourceBufferOffset, 
		_Out_ D3D12XBOX_COUNTER_DATA* pCounterData);

	D3DCONST D3D12CounterSet_Function_Table D3D__D3d12CounterSet_Function_Table = 
	{
		{
			{
				{
					D3d12Object_QueryInterface,
					D3d12DeviceChild_AddRef,
					D3d12CounterSet_Release,
				},
				D3d12Object_GetPrivateData,
				D3d12Object_SetPrivateData,
				D3d12Object_SetPrivateDataInterface,
				D3d12Object_SetName,
			},

			D3d12DeviceChild_GetDevice,
		},

		D3D12CounterSet_GetCounterDataSizeX, 
		D3D12CounterSet_RetrieveCounterDataX, 
	};

#endif
}

D3DINTERFACE(ID3D12XboxCounterSet, 9598d782,875d,4fa0,a0,8e,dc,25,ec,17,d0,2c) : public ID3D12DeviceChild
{
public:

	D3DINLINE D3D12CounterSet_Function_Table* CounterSetFunction()
	{
		return (D3D12CounterSet_Function_Table*)m_pFunction;
	}

	D3DINLINE SIZE_T D3DAPI GetCounterDataSizeX()
	{
		return CounterSetFunction()->GetCounterDataSizeX(this);
	}

	D3DINLINE HRESULT D3DAPI RetrieveCounterDataX(
		_In_ ID3D12Resource* pSourceBuffer,
		_In_ UINT64 AlignedSourceBufferOffset, 
		_Out_ D3D12XBOX_COUNTER_DATA* pCounterData)
	{
		return CounterSetFunction()->RetrieveCounterDataX(this, 
			pSourceBuffer, 
			AlignedSourceBufferOffset, 
			pCounterData);
	}
};
