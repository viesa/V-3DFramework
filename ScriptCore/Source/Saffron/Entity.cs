﻿using System;
using System.Collections.Generic;
using System.Runtime.CompilerServices;

namespace Se
{
    public class Entity
    {
        public ulong ID { get; private set; }

        private List<Action<float>> m_Collision2DBeginCallbacks = new List<Action<float>>();
        private List<Action<float>> m_Collision2DEndCallbacks = new List<Action<float>>();
        private List<Action<float>> m_Collision3DBeginCallbacks = new List<Action<float>>();
        private List<Action<float>> m_Collision3DEndCallbacks = new List<Action<float>>();

        protected Entity() { ID = 0; }

        internal Entity(ulong id)
        {
            ID = id;
        }

        ~Entity()
        {
        }

        public T CreateComponent<T>() where T : Component, new()
        {
            CreateComponent_Native(ID, typeof(T));
            T component = new T();
            component.Entity = this;
            return component;
        }

        public bool HasComponent<T>() where T : Component, new()
        {
            return HasComponent_Native(ID, typeof(T));
        }

        public T GetComponent<T>() where T : Component, new()
        {
            if (HasComponent<T>())
            {
                T component = new T();
                component.Entity = this;
                return component;
            }
            return null;
        }

        public Entity GetEntity(string tag)
        {
            ulong entityID = GetEntity_Native(tag);
            return new Entity(entityID);
        }

        public Matrix4 GetTransform()
        {
            GetTransform_Native(ID, out var mat4Instance);
            return mat4Instance;
        }

        public void SetTransform(Matrix4 transform)
        {
            SetTransform_Native(ID, ref transform);
        }

        public void AddCollision2DBeginCallback(Action<float> callback)
        {
            m_Collision2DBeginCallbacks.Add(callback);
        }

        public void AddCollision2DEndCallback(Action<float> callback)
        {
            m_Collision2DEndCallbacks.Add(callback);
        }
        public void AddCollision3DBeginCallback(Action<float> callback)
        {
            m_Collision3DBeginCallbacks.Add(callback);
        }

        public void AddCollision3DEndCallback(Action<float> callback)
        {
            m_Collision3DEndCallbacks.Add(callback);
        }

        private void OnCollision2DBegin(float data)
        {
            foreach (var callback in m_Collision2DBeginCallbacks)
                callback.Invoke(data);
        }

        private void OnCollision2DEnd(float data)
        {
            foreach (var callback in m_Collision2DEndCallbacks)
                callback.Invoke(data);
        }
        private void OnCollision3DBegin(float data)
        {
            foreach (var callback in m_Collision3DBeginCallbacks)
                callback.Invoke(data);
        }

        private void OnCollision3DEnd(float data)
        {
            foreach (var callback in m_Collision3DEndCallbacks)
                callback.Invoke(data);
        }

        [MethodImpl(MethodImplOptions.InternalCall)]
        private static extern void CreateComponent_Native(ulong entityID, Type type);
        [MethodImpl(MethodImplOptions.InternalCall)]
        private static extern bool HasComponent_Native(ulong entityID, Type type);
        [MethodImpl(MethodImplOptions.InternalCall)]
        private static extern void GetTransform_Native(ulong entityID, out Matrix4 matrix);
        [MethodImpl(MethodImplOptions.InternalCall)]
        private static extern void SetTransform_Native(ulong entityID, ref Matrix4 matrix);
        [MethodImpl(MethodImplOptions.InternalCall)]
        private static extern ulong GetEntity_Native(string tag);

    }
}
